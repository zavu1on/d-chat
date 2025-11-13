#pragma once

#include <memory>

#include "ChatService.hpp"
#include "ConsoleUI.hpp"
#include "IChatClient.hpp"
#include "Message.hpp"
#include "PeerService.hpp"
#include "SocketClient.hpp"

class TCPClient : public IChatClient
{
private:
    SocketClient client;
    std::shared_ptr<IConfig> config;
    std::shared_ptr<ICrypto> crypto;
    std::shared_ptr<ChatService> chatService;
    std::shared_ptr<PeerService> peerService;
    std::shared_ptr<ConsoleUI> consoleUI;

public:
    TCPClient(const std::shared_ptr<IConfig>& config,
              const std::shared_ptr<ICrypto>& crypto,
              const std::shared_ptr<ChatService>& chatService,
              const std::shared_ptr<PeerService>& peerService,
              const std::shared_ptr<ConsoleUI>& consoleUI);
    ~TCPClient() override;

    void sendMessage(const Message& message, bool withSecret = false) override;
};