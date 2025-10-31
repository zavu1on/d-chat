#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>

class SocketClient
{
protected:
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;
    std::string host;
    unsigned short port;
    bool connected;

    bool connectToServer();

public:
    SocketClient(const std::string& host, int port, bool connectImmediately = true);
    ~SocketClient();

    SocketClient(const SocketClient&) = delete;
    SocketClient& operator=(const SocketClient&) = delete;
    SocketClient(SocketClient&& other) noexcept;
    SocketClient& operator=(SocketClient&& other) noexcept;

    bool sendMessage(const std::string& message);
    std::string receiveMessage();

    void disconnect();
    bool reconnect();
    bool isConnected() const;

    std::string getHost() const;
    unsigned short getPort() const;
};
