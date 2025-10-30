#include "TCPServer.hpp"

TCPServer::TCPServer(unsigned short port, const std::shared_ptr<ChatService>& chatService)
    : server(port), chatService(chatService)
{
}

void TCPServer::start()
{
    server.startAsync(
        [this](const char* buffer, int bufferSize,
               std::function<void(const std::string&)> sendCallback)
        {
            std::string message(buffer, bufferSize);
            std::string response = chatService->handleIncoming(message);

            sendCallback(response);
        });
}

void TCPServer::stop() { server.stop(); }

bool TCPServer::listening() const { return server.listening(); }
