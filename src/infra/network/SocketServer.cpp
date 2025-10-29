#include "SocketServer.hpp"

void SocketServer::listenMessages(MessageHandler onMessage)
{
    // listen max queued connections
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
        throw std::runtime_error("Failed to listen on socket");

    isListening.store(true, std::memory_order_relaxed);

    while (isListening.load(std::memory_order_relaxed))
    {
        sockaddr_in clientAddr{};
        int clientSize = sizeof(clientAddr);

        // accept client connection
        SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == INVALID_SOCKET) continue;

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));

        char buffer[BUFFER_SIZE];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0)
        {
            closesocket(clientSocket);
            continue;
        }

        buffer[bytesReceived] = '\0';

        auto sendFunc = [clientSocket](const std::string& msg)
        { send(clientSocket, msg.c_str(), static_cast<int>(msg.size()), 0); };

        try
        {
            onMessage(buffer, bytesReceived, sendFunc);
        }
        catch (...)
        {
            isListening.store(false, std::memory_order_relaxed);
            closesocket(clientSocket);
            throw std::runtime_error("Failed to handle message");
        }

        // close one connection, ready for next
        closesocket(clientSocket);
    }
}

SocketServer::SocketServer(int port)
{
    // init winsock2.dll
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) throw std::runtime_error("Failed to initialize Winsock");

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);  // tcp socket
    if (listenSocket == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // bind server address to socket
    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        WSACleanup();
        throw std::runtime_error("Failed to bind socket");
    }

    isListening.store(false, std::memory_order_relaxed);
}

SocketServer::~SocketServer()
{
    stop();

    closesocket(listenSocket);
    WSACleanup();
}

bool SocketServer::start(MessageHandler onMessage)
{
    if (!isListening.load(std::memory_order_relaxed))
    {
        listenThread = std::thread(&SocketServer::listenMessages, this, onMessage);
        listenThread.detach();

        return true;
    }
    return false;
}

void SocketServer::stop()
{
    if (listenThread.joinable()) listenThread.join();
    isListening.store(false, std::memory_order_relaxed);
}

bool SocketServer::listening() const { return isListening.load(std::memory_order_relaxed); }
