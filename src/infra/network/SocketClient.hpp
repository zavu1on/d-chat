#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

#pragma comment(lib, "ws2_32.lib")

class SocketClient
{
protected:
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;

public:
    SocketClient(const std::string& host, int port);
    ~SocketClient();

    void sendMessage(const std::string& message);
    std::string receiveMessage();
};
