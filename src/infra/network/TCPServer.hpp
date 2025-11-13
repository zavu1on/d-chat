#pragma once

#include <memory>

#include "ChatService.hpp"
#include "IChatServer.hpp"
#include "SocketServer.hpp"

namespace network
{
class TCPServer : public IChatServer
{
private:
    SocketServer server;
    std::shared_ptr<chat::ChatService> chatService;

public:
    TCPServer(u_short port, const std::shared_ptr<chat::ChatService>& chatService);

    void start() override;
    void stop() override;
    bool listening() const;
};
}  // namespace network