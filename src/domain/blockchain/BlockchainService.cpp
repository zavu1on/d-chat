#include "BlockchainService.hpp"

#include <openssl/sha.h>

#include "BlockchainErrorMessage.hpp"
#include "sha256.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

namespace blockchain
{
bool BlockchainService::verifyBlockSignature(const Block& block)
{
    std::string canon = block.toStringForHash();

    crypto::Bytes msg(canon.begin(), canon.end());
    crypto::Bytes sig = crypto->stringToKey(block.signature);
    crypto::Bytes pub = crypto->stringToKey(block.authorPublicKey);

    bool ok = crypto->verify(msg, sig, pub);

    if (!ok)
    {
        consoleUI->printLog("[BLOCKCHAIN] Invalid signature: " + block.hash + "\n");
        return false;
    }

    return true;
}

bool BlockchainService::validateSingleBlock(const Block& block, std::string& error)
{
    if (!verifyBlockSignature(block))
    {
        error = "Invalid block signature";
        return false;
    }

    Block tempBlock = block;
    tempBlock.hash = "";
    tempBlock.computeHash();

    if (tempBlock.hash != block.hash)
    {
        error = "Block hash mismatch: computed=" + tempBlock.hash + ", stored=" + block.hash;
        return false;
    }

    return true;
}

inline void BlockchainService::logValidationError(const std::string& context,
                                                  const std::string& error,
                                                  const std::string& blockHash)
{
    consoleUI->printLog("[BLOCKCHAIN] " + context + " validation failed for block " + blockHash +
                        ": " + error + "\n");
}

BlockchainService::BlockchainService(const std::shared_ptr<config::IConfig>& config,
                                     const std::shared_ptr<crypto::ICrypto>& crypto,
                                     const std::shared_ptr<IChainRepo>& chainRepo,
                                     const std::shared_ptr<ui::ConsoleUI>& consoleUI)
    : config(config), crypto(crypto), chainRepo(chainRepo), consoleUI(consoleUI), newBlocks()
{
}

std::vector<Block> BlockchainService::getNewBlocks() const
{
    std::lock_guard<std::mutex> lock(newBlocksMutex);
    return newBlocks;
}

void BlockchainService::addNewBlockRange(const std::vector<Block>& blocks)
{
    std::lock_guard<std::mutex> lock(newBlocksMutex);
    newBlocks.insert(newBlocks.end(), blocks.begin(), blocks.end());
}

void BlockchainService::createBlockFromMessage(const message::TextMessage& message, Block& block)
{
    Block tip;
    chainRepo->findTip(tip);
    block.previousHash = tip.hash;

    block.payloadHash = utils::sha256(message.getPayload().message.data());

    block.authorPublicKey = message.getFrom().publicKey;
    block.timestamp = message.getTimestamp();

    std::string privateKey = config->get(config::ConfigField::PRIVATE_KEY);
    std::string canonical = block.toStringForHash();
    crypto::Bytes canonicalBytes(canonical.begin(), canonical.end());
    crypto::Bytes priv = crypto->stringToKey(privateKey);
    crypto::Bytes sign = crypto->sign(canonicalBytes, priv);
    block.signature = crypto->keyToString(sign);

    block.computeHash();
}

bool BlockchainService::storeAndBroadcastBlock(
    const Block& block,
    const std::vector<peer::UserPeer>& peers,
    const std::function<bool(const std::string&, const peer::UserPeer&)>& sendCallback)
{
    bool ok = chainRepo->insertBlock(block);
    if (!ok) return false;

    std::string rawJson = block.toJson().dump();

    for (const auto& p : peers)
    {
        try
        {
            sendCallback(rawJson, p);
        }
        catch (std::exception& error)
        {
            consoleUI->printLog(
                "[BLOCKCHAIN] error sending block to peer: " + std::string(error.what()) + "\n");
        }
    }

    return true;
}

void BlockchainService::onIncomingBlock(const json& jData, std::string& response)
{
    try
    {
        Block block(jData);
        std::string error;

        if (!validateIncomingBlock(block, error))
        {
            peer::UserPeer me(
                config->get(config::ConfigField::HOST),
                static_cast<unsigned short>(stoi(config->get(config::ConfigField::PORT))),
                config->get(config::ConfigField::PUBLIC_KEY));

            message::BlockchainErrorMessageResponse errorResponse(
                utils::uuidv4(), {}, utils::getTimestamp(), error, block);

            json jData;
            errorResponse.serialize(jData);

            response = jData.dump();
            return;
        }

        chainRepo->insertBlock(block);
        response = "{}";
    }
    catch (const std::exception& e)
    {
        consoleUI->printLog("[BLOCKCHAIN] Exception parsing block: " + std::string(e.what()) +
                            "\n");
        response = "{}";
    }
}

void BlockchainService::loadChain(std::vector<Block>& blocks) { chainRepo->loadAllBlocks(blocks); }

bool BlockchainService::validateLocalChain()
{
    std::vector<Block> blocks;
    chainRepo->loadAllBlocks(blocks);

    if (blocks.empty()) return true;

    bool isValid = true;
    std::string prevHash = "0";

    for (size_t i = 0; i < blocks.size(); ++i)
    {
        Block& block = blocks[i];
        std::string error;

        if (block.previousHash != prevHash)
        {
            error =
                "Previous hash mismatch: expected=" + prevHash + ", actual=" + block.previousHash;
            isValid = false;
            logValidationError("LOCAL_CHAIN", error, block.hash);
        }
        else if (!validateSingleBlock(block, error))
        {
            isValid = false;
            logValidationError("LOCAL_CHAIN", error, block.hash);
        }

        prevHash = block.hash;
    }

    if (isValid)
        consoleUI->printLog("[BLOCKCHAIN] Local chain validated successfully (" +
                            std::to_string(blocks.size()) + " blocks)\n");

    return isValid;
}

bool BlockchainService::validateIncomingBlock(const Block& block, std::string& error)
{
    if (!validateSingleBlock(block, error))
    {
        return false;
    }
    uint64_t currentTimestamp = utils::getTimestamp();
    if (block.timestamp > currentTimestamp + 300000)  // this time + 5 minutes
    {
        error = "Block timestamp is too far in the future";
        return false;
    }

    Block tip;
    if (chainRepo->findTip(tip) && block.previousHash != tip.hash)
    {
        Block prevBlock;
        if (!chainRepo->findBlockByHash(block.previousHash, prevBlock))
        {
            error = "Previous block not found in local chain";
            return false;
        }

        if (prevBlock.hash != tip.hash)
        {
            error = "Block extends non-tip block (fork detected)";
            return false;
        }
    }

    if (chainRepo->hasBlock(block.hash))
    {
        error = "Block already exists in local chain";
        return false;
    }

    return true;
}

bool BlockchainService::validateNewBlocks()
{
    std::lock_guard<std::mutex> lock(newBlocksMutex);

    if (newBlocks.empty()) return true;

    for (size_t i = 1; i < newBlocks.size(); ++i)
    {
        if (newBlocks[i].previousHash != newBlocks[i - 1].hash)
        {
            std::string error = "New blocks sequence broken at index " + std::to_string(i) +
                                ": expected=" + newBlocks[i - 1].hash +
                                ", actual=" + newBlocks[i].previousHash;
            logValidationError("NEW_BLOCKS", error, newBlocks[i].hash);
            return false;
        }
    }

    Block tip;
    bool found = chainRepo->findTip(tip);

    if (found && newBlocks[0].previousHash != tip.hash)
    {
        std::string error;
        Block prevBlock;

        if (!chainRepo->findBlockByHash(newBlocks[0].previousHash, prevBlock))
        {
            error = "First new block's previous hash not found in local chain";
            logValidationError("NEW_BLOCKS", error, newBlocks[0].hash);
            return false;
        }

        if (prevBlock.hash != tip.hash)
        {
            error = "New blocks extend non-tip block (fork detected)";
            logValidationError("NEW_BLOCKS", error, newBlocks[0].hash);
            return false;
        }
    }

    bool allValid = true;
    for (size_t i = 0; i < newBlocks.size(); ++i)
    {
        std::string error;
        if (!validateSingleBlock(newBlocks[i], error))
        {
            logValidationError("NEW_BLOCKS", error, newBlocks[i].hash);
            allValid = false;
            break;
        }
    }

    return allValid;
}

bool BlockchainService::compareBlockWithMessage(const Block& block,
                                                const message::TextMessage& message,
                                                std::string& error)
{
    if (block.authorPublicKey != message.getFrom().publicKey)
    {
        error = "Author public key mismatch: block=" + block.authorPublicKey +
                ", message=" + message.getFrom().publicKey;
        return false;
    }

    if (block.timestamp != message.getTimestamp())
    {
        error = "Timestamp mismatch: block=" + std::to_string(block.timestamp) +
                ", message=" + std::to_string(message.getTimestamp());
        return false;
    }

    std::string messageContent = message.getPayload().message;
    std::string computedPayloadHash = utils::sha256(messageContent.data());

    if (computedPayloadHash != block.payloadHash)
    {
        error = "Payload hash mismatch: block=" + block.payloadHash +
                ", computed=" + computedPayloadHash + ", content='" + messageContent + "'";
        return false;
    }

    if (!verifyBlockSignature(block))
    {
        error = "Block signature verification failed";
        return false;
    }

    return true;
}

u_int BlockchainService::countBlocksAfterHash(const std::string& hash)
{
    return chainRepo->countBlocksAfterHash(hash);
}

void BlockchainService::getBlocksByIndexRange(u_int start,
                                              u_int count,
                                              const std::string& lastHash,
                                              std::vector<Block>& outBlocks)
{
    chainRepo->getBlocksByIndexRange(start, count, lastHash, outBlocks);
}

}  // namespace blockchain