#pragma once

#include <memory>
#include <nlohmann/json.hpp>

#include "BlockchainService.hpp"
#include "ChatService.hpp"
#include "ConsoleUI.hpp"
#include "IChatServer.hpp"
#include "SocketServer.hpp"

namespace network
{
using json = nlohmann::json;

class TCPServer : public IChatServer
{
private:
    SocketServer server;
    std::shared_ptr<chat::ChatService> chatService;
    std::shared_ptr<blockchain::BlockchainService> blockchainService;
    std::shared_ptr<ui::ConsoleUI> consoleUI;

public:
    TCPServer(u_short port,
              const std::shared_ptr<chat::ChatService>& chatService,
              const std::shared_ptr<blockchain::BlockchainService>& blockchainService,
              const std::shared_ptr<ui::ConsoleUI>& consoleUI);

    void start() override;
    void stop() override;
    bool listening() const;
};
}  // namespace network