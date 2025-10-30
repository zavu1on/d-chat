#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <functional>
#include <stdexcept>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

const int BUFFER_SIZE = 512;

class SocketServer
{
private:
    using MessageHandler = std::function<void(
        const char* buffer, int bufferSize, std::function<void(const std::string&)> sendCallback)>;

    void listenMessages(MessageHandler onMessage);

protected:
    WSADATA wsaData;
    SOCKET listenSocket;
    sockaddr_in serverAddr;
    std::thread listenThread;
    std::atomic<bool> isListening;  // atomic to avoid race conditions

public:
    explicit SocketServer(unsigned short port);
    ~SocketServer();

    SocketServer(const SocketServer&) = delete;
    SocketServer& operator=(const SocketServer&) = delete;
    SocketServer(SocketServer&& other) noexcept;
    SocketServer& operator=(SocketServer&& other) noexcept;

    bool startAsync(MessageHandler onMessage);
    void stop();
    bool listening() const;
    unsigned short getPort() const;
};
