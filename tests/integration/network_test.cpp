#include <gtest/gtest.h>

#include <atomic>
#include <thread>

#include "SocketClient.hpp"
#include "SocketServer.hpp"
#include "test_helpers.hpp"


class NetworkTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    }

    void TearDown() override { WSACleanup(); }
};

TEST_F(NetworkTest, SocketClientConnectAndDisconnectSucceeds)
{
    std::atomic<bool> serverReady(false);
    std::thread serverThread(
        [&serverReady]()
        {
            network::SocketServer server(test_helpers::TEST_PORT_PEER1);
            serverReady.store(true);

            server.startAsync([](const char*, int, std::function<void(const std::string&)> send)
                              { send("ACK"); });

            std::this_thread::sleep_for(std::chrono::seconds(2));
            server.stop();
        });

    // Wait for server to be ready
    while (!serverReady.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    network::SocketClient client;
    EXPECT_TRUE(client.connectTo("127.0.0.1", test_helpers::TEST_PORT_PEER1));
    EXPECT_TRUE(client.isConnected());

    client.disconnect();
    EXPECT_FALSE(client.isConnected());

    serverThread.join();
}

TEST_F(NetworkTest, SocketClientSendAndReceiveSucceeds)
{
    std::atomic<bool> messageReceived(false);
    std::string receivedMessage;

    std::thread serverThread(
        [&messageReceived, &receivedMessage]()
        {
            network::SocketServer server(test_helpers::TEST_PORT_PEER2);

            server.startAsync(
                [&messageReceived, &receivedMessage](const char* buffer,
                                                     int bufferSize,
                                                     std::function<void(const std::string&)> send)
                {
                    receivedMessage = std::string(buffer, bufferSize);
                    messageReceived.store(true);
                    send("Response from server");
                });

            std::this_thread::sleep_for(std::chrono::seconds(2));
            server.stop();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    network::SocketClient client;
    EXPECT_TRUE(client.connectTo("127.0.0.1", test_helpers::TEST_PORT_PEER2));

    std::string testMessage = "Test message from client";
    EXPECT_TRUE(client.sendMessage(testMessage));

    std::string response = client.receiveMessage();
    EXPECT_EQ(response, "Response from server");

    // Wait for server to process message
    auto start = std::chrono::steady_clock::now();
    while (!messageReceived.load() && std::chrono::duration_cast<std::chrono::seconds>(
                                          std::chrono::steady_clock::now() - start)
                                              .count() < 2)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(messageReceived.load());
    EXPECT_EQ(receivedMessage, testMessage);

    client.disconnect();
    serverThread.join();
}

TEST_F(NetworkTest, SocketServerHandlesMultipleConnections)
{
    std::atomic<int> connectionCount(0);

    std::thread serverThread(
        [&connectionCount]()
        {
            network::SocketServer server(test_helpers::TEST_PORT_PEER3);

            server.startAsync(
                [&connectionCount](const char*, int, std::function<void(const std::string&)> send)
                {
                    connectionCount.fetch_add(1);
                    send("ACK");
                });

            std::this_thread::sleep_for(std::chrono::seconds(2));
            server.stop();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Create multiple client connections
    std::vector<std::thread> clientThreads;
    const int numClients = 5;

    for (int i = 0; i < numClients; ++i)
    {
        clientThreads.emplace_back(
            []()
            {
                network::SocketClient client;
                if (client.connectTo("127.0.0.1", test_helpers::TEST_PORT_PEER3))
                {
                    client.sendMessage("Hello");
                    client.receiveMessage();
                    client.disconnect();
                }
            });
    }

    for (auto& thread : clientThreads)
    {
        thread.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(connectionCount.load(), numClients);

    serverThread.join();
}

TEST_F(NetworkTest, SocketClientConnectToNonExistentServerFails)
{
    network::SocketClient client;
    EXPECT_FALSE(client.connectTo("127.0.0.1", 19999));  // Non-existent port
    EXPECT_FALSE(client.isConnected());
}

TEST_F(NetworkTest, FindFreePortReturnsValidPort)
{
    unsigned short port = network::SocketClient::findFreePort();

    EXPECT_GT(port, 0);
    EXPECT_LT(port, 65536);

    // Verify port is actually free by binding to it
    network::SocketServer server(port);
    EXPECT_NO_THROW(
        server.startAsync([](const char*, int, std::function<void(const std::string&)>) {}));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server.stop();
}