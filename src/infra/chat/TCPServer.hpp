#pragma once

#include <memory>

#include "ChatService.hpp"
#include "IChatServer.hpp"
#include "SocketServer.hpp"

class TCPServer : public IChatServer
{
private:
    network::SocketServer server;
    std::shared_ptr<ChatService> chatService;

public:
    TCPServer(u_short port, const std::shared_ptr<ChatService>& chatService);

    void start() override;
    void stop() override;
    bool listening() const;
};