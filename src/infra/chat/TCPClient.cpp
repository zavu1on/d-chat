#include "TCPClient.hpp"

#include <stdexcept>

TCPClient::TCPClient(const std::string& host, int port,
                     const std::shared_ptr<ChatService>& chatService)
    : client(host, port, false), chatService(chatService)
{
}

std::string TCPClient::sendMessage(const std::string& message)
{
    if (!client.isConnected()) client.reconnect();

    chatService->handleOutgoing(message);

    client.sendMessage(message);
    std::string response = client.receiveMessage();
    client.disconnect();

    return response;
}
