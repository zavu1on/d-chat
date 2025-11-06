#pragma once

#include <memory>

#include "ChatService.hpp"
#include "IChatClient.hpp"
#include "SocketClient.hpp"

class TCPClient : public IChatClient
{
private:
    SocketClient client;
    std::shared_ptr<ChatService> chatService;

public:
    TCPClient(const std::string& host, int port, const std::shared_ptr<ChatService>& chatService);

    std::string sendMessage(const std::string& message) override;
};