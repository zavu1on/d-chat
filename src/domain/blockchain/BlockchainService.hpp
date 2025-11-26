#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "ConsoleUI.hpp"
#include "IChainRepo.hpp"
#include "IConfig.hpp"
#include "ICrypto.hpp"
#include "TextMessage.hpp"
#include "UserPeer.hpp"

namespace blockchain
{
class BlockchainService
{
private:
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<IChainRepo> chainRepo;
    std::shared_ptr<ui::ConsoleUI> consoleUI;

    std::vector<Block> newBlocks;
    mutable std::mutex newBlocksMutex;

    bool verifyBlockSignature(const Block& block);
    bool validateSingleBlock(const Block& block, std::string& error);
    inline void logValidationError(const std::string& context,
                                   const std::string& error,
                                   const std::string& blockHash);

public:
    BlockchainService(const std::shared_ptr<config::IConfig>& config,
                      const std::shared_ptr<crypto::ICrypto>& crypto,
                      const std::shared_ptr<IChainRepo>& chainRepo,
                      const std::shared_ptr<ui::ConsoleUI>& consoleUI);

    std::vector<Block> getNewBlocks() const;
    void addNewBlockRange(const std::vector<Block>& blocks);
    void createBlockFromMessage(const message::TextMessage& message,

                                Block& block);
    bool storeAndBroadcastBlock(
        const Block& block,
        const std::vector<peer::UserPeer>& peers,
        const std::function<bool(const std::string&, const peer::UserPeer&)>& sendCallback);
    void onIncomingBlock(const json& jData, std::string& response);
    void loadChain(std::vector<Block>& blocks);

    bool validateLocalChain();
    bool validateIncomingBlock(const Block& block, std::string& error);
    bool validateNewBlocks();
    bool compareBlockWithMessage(const Block& block,
                                 const message::TextMessage& message,
                                 std::string& error);

    u_int countBlocksAfterHash(const std::string& hash);
    void getBlocksByIndexRange(u_int start,
                               u_int count,
                               const std::string& lastHash,
                               std::vector<Block>& outBlocks);
};
}  // namespace blockchain
