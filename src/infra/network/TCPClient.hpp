#pragma once

#include <memory>
#include <nlohmann/json.hpp>

#include "ChatService.hpp"
#include "ConsoleUI.hpp"
#include "IChatClient.hpp"
#include "Message.hpp"
#include "PeerService.hpp"
#include "SocketClient.hpp"

namespace network
{
using json = nlohmann::json;

class TCPClient : public IChatClient
{
private:
    SocketClient client;
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<chat::ChatService> chatService;
    std::shared_ptr<peer::PeerService> peerService;
    std::shared_ptr<ui::ConsoleUI> consoleUI;

public:
    TCPClient(const std::shared_ptr<config::IConfig>& config,
              const std::shared_ptr<crypto::ICrypto>& crypto,
              const std::shared_ptr<chat::ChatService>& chatService,
              const std::shared_ptr<peer::PeerService>& peerService,
              const std::shared_ptr<ui::ConsoleUI>& consoleUI);

    void connectToAllPeers() override;
    void sendMessage(const message::Message& message, bool withSecret = false) override;
    void disconnect() override;
};
}  // namespace network