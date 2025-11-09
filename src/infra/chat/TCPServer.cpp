#include "TCPServer.hpp"

TCPServer::TCPServer(u_short port, const std::shared_ptr<ChatService>& chatService)
    : server(port), chatService(chatService)
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

            std::string response;
            chatService->handleIncomingMessage(message, response);

            sendCallback(response);
        });
    if (!started) throw std::runtime_error("Failed to start server");
}

void TCPServer::stop() { server.stop(); }

bool TCPServer::listening() const { return server.listening(); }
