#pragma once

#include <memory>
#include <nlohmann/json.hpp>

#include "BlockchainService.hpp"
#include "ChatService.hpp"
#include "ConsoleUI.hpp"
#include "IChatClient.hpp"
#include "Message.hpp"
#include "MessageService.hpp"
#include "PeerService.hpp"
#include "SocketClient.hpp"

namespace network
{
using json = nlohmann::json;

class TCPClient : public IChatClient
{
private:
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<chat::ChatService> chatService;
    std::shared_ptr<peer::PeerService> peerService;
    std::shared_ptr<blockchain::BlockchainService> blockchainService;
    std::shared_ptr<message::MessageService> messageService;
    std::shared_ptr<ui::ConsoleUI> consoleUI;

public:
    TCPClient(const std::shared_ptr<config::IConfig>& config,
              const std::shared_ptr<crypto::ICrypto>& crypto,
              const std::shared_ptr<chat::ChatService>& chatService,
              const std::shared_ptr<peer::PeerService>& peerService,
              const std::shared_ptr<blockchain::BlockchainService>& blockchainService,
              const std::shared_ptr<message::MessageService>& messageService,
              const std::shared_ptr<ui::ConsoleUI>& consoleUI,
              const std::string& lastHash);

    void connectToAllPeers() override;
    void sendMessage(const message::Message& message) override;
    void sendSecretMessage(const message::SecretMessage& message) override;
    void disconnect() override;
};
}  // namespace network