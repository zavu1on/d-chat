#pragma once

#include <memory>

#include "ChatService.hpp"
#include "IChatServer.hpp"
#include "SocketServer.hpp"

class TCPServer : public IChatServer
{
private:
    SocketServer server;
    std::shared_ptr<ChatService> chatService;

public:
    TCPServer(unsigned short port, const std::shared_ptr<ChatService>& chatService);

    void start() override;
    void stop() override;
    bool listening() const;
};