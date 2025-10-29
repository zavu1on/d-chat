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
    std::atomic<bool> isListening;

public:
    explicit SocketServer(int port);
    ~SocketServer();

    bool start(MessageHandler onMessage);
    void stop();
    bool listening() const;
};
