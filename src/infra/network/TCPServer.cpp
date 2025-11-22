#include "TCPServer.hpp"

namespace network
{
TCPServer::TCPServer(u_short port,
                     const std::shared_ptr<chat::ChatService>& chatService,
                     const std::shared_ptr<blockchain::BlockchainService>& blockchainService,
                     const std::shared_ptr<ui::ConsoleUI>& consoleUI)
    : server(port),
      chatService(chatService),
      blockchainService(blockchainService),
      consoleUI(consoleUI)
{
}

void TCPServer::start()
{
    bool started = server.startAsync(
        [this](const char* buffer,
               int bufferSize,
               std::function<void(const std::string&)> sendCallback)
        {
            std::string message(buffer, bufferSize);

            try
            {
                json jData = json::parse(message);
                std::string response = "{}";

                if (jData.contains("previousHash") && jData.contains("payloadHash"))
                {
                    blockchainService->onIncomingBlock(jData, response);
                }
                else
                {
                    chatService->handleIncomingMessage(jData, response);
                }

                sendCallback(response);
            }
            catch (std::exception& error)
            {
                consoleUI->printLog(
                    "[SERVER] handle incoming message error: " + std::string(error.what()) + "\n");
            }
        });
    if (!started) throw std::runtime_error("Failed to start server");
}

void TCPServer::stop() { server.stop(); }

bool TCPServer::listening() const { return server.listening(); }
}  // namespace network