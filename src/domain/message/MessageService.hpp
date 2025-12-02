#pragma once
#include <memory>
#include <vector>

#include "BlockchainService.hpp"
#include "ConsoleUI.hpp"
#include "IChainRepo.hpp"
#include "IConfig.hpp"
#include "ICrypto.hpp"
#include "IMessageRepo.hpp"
#include "TextMessage.hpp"

namespace message
{
class MessageService
{
private:
    std::shared_ptr<IMessageRepo> messageRepo;
    std::shared_ptr<blockchain::BlockchainService> blockchainService;
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<ui::ConsoleUI> consoleUI;

public:
    MessageService(const std::shared_ptr<IMessageRepo>& messageRepo,
                   const std::shared_ptr<blockchain::BlockchainService>& blockchainService,
                   const std::shared_ptr<config::IConfig>& config,
                   const std::shared_ptr<crypto::ICrypto>& crypto,
                   const std::shared_ptr<ui::ConsoleUI>& consoleUI);

    void findChatMessages(const std::string& peerAPublicKey,
                          const std::string& peerBPublicKey,
                          std::vector<TextMessage>& messages);
    void findInvalidChatMessageIDs(const std::vector<TextMessage>& messages,
                                   std::vector<std::string>& invalidIds);
    bool insertSecretMessage(const TextMessage& message,
                             const std::string& messageDump,
                             const std::string& blockHash);
    bool removeMessageByBlockHash(const std::string& blockHash);
};
}  // namespace message