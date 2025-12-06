#include "MessageService.hpp"

#include "Block.hpp"

namespace message
{
MessageService::MessageService(
    const std::shared_ptr<IMessageRepo>& messageRepo,
    const std::shared_ptr<blockchain::BlockchainService>& blockchainService,
    const std::shared_ptr<config::IConfig>& config,
    const std::shared_ptr<crypto::ICrypto>& crypto,
    const std::shared_ptr<ui::ConsoleUI>& consoleUI)
    : messageRepo(messageRepo),
      blockchainService(blockchainService),
      config(config),
      crypto(crypto),
      consoleUI(consoleUI)
{
}

void MessageService::findChatMessages(const std::string& peerAPublicKey,
                                      const std::string& peerBPublicKey,
                                      std::vector<TextMessage>& messages)
{
    try
    {
        messageRepo->findChatMessages(peerAPublicKey, peerBPublicKey, messages);
    }
    catch (const std::exception&)
    {
        consoleUI->printLog("[ERROR] Failed to find some chat messages. Blockchain was invalid\n");
    }
}

void MessageService::findInvalidChatMessageIDs(const std::vector<TextMessage>& messages,
                                               std::vector<std::string>& invalidIds)
{
    for (const auto& message : messages)
    {
        std::string blockHash;
        if (!messageRepo->findBlockHashByMessageId(message.getId(), blockHash) ||
            blockHash.empty() || blockHash != message.getBlockHash())
        {
            invalidIds.push_back(message.getId());
            continue;
        }

        blockchain::Block block;
        if (!blockchainService->findBlockByHash(blockHash, block))
        {
            invalidIds.push_back(message.getId());
            continue;
        }
        std::cout << "Block hash: " << blockHash << " " << block.hash << std::endl;

        std::string error;
        if (!blockchainService->compareBlockWithMessage(block, message, error))
        {
            invalidIds.push_back(message.getId());
        }
    }
}

bool MessageService::insertSecretMessage(const TextMessage& message,
                                         const std::string& messageDump,
                                         const std::string& blockHash)
{
    return messageRepo->insertSecretMessage(message, messageDump, blockHash);
}

bool MessageService::removeMessageByBlockHash(const std::string& blockHash)
{
    return messageRepo->removeMessageByBlockHash(blockHash);
}

}  // namespace message