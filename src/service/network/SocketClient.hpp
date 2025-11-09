#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdexcept>
#include <string>


class SocketClient
{
protected:
    WSADATA wsaData{};
    SOCKET clientSocket = INVALID_SOCKET;
    sockaddr_in serverAddr{};
    bool initialized = false;
    bool connected = false;

public:
    SocketClient();
    ~SocketClient();

    SocketClient(const SocketClient&) = delete;
    SocketClient& operator=(const SocketClient&) = delete;
    SocketClient(SocketClient&& other) noexcept;
    SocketClient& operator=(SocketClient&& other) noexcept;

    bool connectTo(const std::string& host, u_short port);

    bool sendMessage(const std::string& message);
    std::string receiveMessage();

    void disconnect();
    bool isConnected() const;

    static u_short findFreePort();

private:
    void ensureInitialized();
};
