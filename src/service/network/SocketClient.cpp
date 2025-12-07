#include "SocketClient.hpp"

#include "SocketServer.hpp"


namespace network
{
void SocketClient::ensureInitialized()
{
    if (!initialized)
    {
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) throw std::runtime_error("Failed to initialize Winsock");
        initialized = true;
    }
}

SocketClient::SocketClient() { ensureInitialized(); }

SocketClient::~SocketClient()
{
    disconnect();
    if (initialized)
    {
        WSACleanup();
        initialized = false;
    }
}

SocketClient::SocketClient(SocketClient&& other) noexcept
    : wsaData(other.wsaData),
      clientSocket(other.clientSocket),
      serverAddr(other.serverAddr),
      initialized(other.initialized),
      connected(other.connected)
{
    other.clientSocket = INVALID_SOCKET;
    other.connected = false;
    other.initialized = false;
}

SocketClient& SocketClient::operator=(SocketClient&& other) noexcept
{
    if (this != &other)
    {
        disconnect();

        wsaData = other.wsaData;
        clientSocket = other.clientSocket;
        serverAddr = other.serverAddr;
        initialized = other.initialized;
        connected = other.connected;

        other.clientSocket = INVALID_SOCKET;
        other.connected = false;
        other.initialized = false;
    }
    return *this;
}

bool SocketClient::connectTo(const std::string& host, u_short port)
{
    disconnect();
    ensureInitialized();

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) return false;

    serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        return false;
    }

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) ==
        SOCKET_ERROR)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
        connected = false;
        return false;
    }

    connected = true;
    return true;
}

bool SocketClient::sendMessage(const std::string& message)
{
    if (!connected) return false;
    int result = send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0);
    return result != SOCKET_ERROR;
}

std::string SocketClient::receiveMessage()
{
    if (!connected) throw std::runtime_error("Not connected to server");

    char buffer[BUFFER_SIZE];
    int bytes = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);

    if (bytes <= 0)
    {
        connected = false;
        return "";
    }

    buffer[bytes] = '\0';
    return std::string(buffer);
}

void SocketClient::disconnect()
{
    if (clientSocket != INVALID_SOCKET)
    {
        closesocket(clientSocket);
        clientSocket = INVALID_SOCKET;
    }
    connected = false;
}

bool SocketClient::isConnected() const { return connected; }

unsigned short SocketClient::findFreePort()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) throw std::runtime_error("WSAStartup failed");

    SOCKET tempSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tempSocket == INVALID_SOCKET)
    {
        WSACleanup();
        throw std::runtime_error("Failed to create socket");
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;

    if (bind(tempSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        closesocket(tempSocket);
        WSACleanup();
        throw std::runtime_error("Bind failed");
    }

    sockaddr_in boundAddr{};
    int addrLen = sizeof(boundAddr);
    if (getsockname(tempSocket, reinterpret_cast<sockaddr*>(&boundAddr), &addrLen) == SOCKET_ERROR)
    {
        closesocket(tempSocket);
        WSACleanup();
        throw std::runtime_error("getsockname failed");
    }

    unsigned short freePort = ntohs(boundAddr.sin_port);

    closesocket(tempSocket);
    WSACleanup();
    return freePort;
}
}  // namespace network