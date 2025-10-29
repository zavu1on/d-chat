#include "SocketClient.hpp"

#include <stdexcept>


SocketClient::SocketClient(const std::string& host, int port)
{
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) throw std::runtime_error("Failed to initialize Winsock");

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(clientSocket);
        WSACleanup();
        throw std::runtime_error("Failed to connect to server");
    }
}

SocketClient::~SocketClient()
{
    closesocket(clientSocket);
    WSACleanup();
}

void SocketClient::sendMessage(const std::string& message)
{
    send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0);
}

std::string SocketClient::receiveMessage()
{
    char buffer[512];
    int bytes = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return "";

    buffer[bytes] = '\0';
    return std::string(buffer);
}
