#include <gtest/gtest.h>

#include <atomic>
#include <thread>

#include "ChainDB.hpp"
#include "JsonConfig.hpp"
#include "MessageDB.hpp"
#include "OpenSSLCrypto.hpp"
#include "PeerDB.hpp"
#include "TCPClient.hpp"
#include "TCPServer.hpp"
#include "test_helpers.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

class FullChatScenarioTest : public ::testing::Test
{
protected:
    std::unique_ptr<test_helpers::TestEnvironment> env;
    std::shared_ptr<crypto::ICrypto> crypto;

    void SetUp() override
    {
        env = std::make_unique<test_helpers::TestEnvironment>();
        crypto = std::make_shared<crypto::OpenSSLCrypto>();
    }

    void TearDown() override { env->cleanup(); }

    struct PeerSetup
    {
        unsigned short port;
        std::string configPath;
        std::string dbPath;
        crypto::KeyPair keyPair;
        std::shared_ptr<config::IConfig> config;
        std::shared_ptr<db::DBFile> db;
        std::shared_ptr<blockchain::IChainRepo> chainRepo;
        std::shared_ptr<message::IMessageRepo> messageRepo;
        std::shared_ptr<peer::IPeerRepo> peerRepo;
        std::shared_ptr<peer::PeerService> peerService;
        std::shared_ptr<blockchain::BlockchainService> blockchainService;
        std::shared_ptr<message::MessageService> messageService;
        std::shared_ptr<chat::ChatService> chatService;
        std::shared_ptr<network::TCPServer> server;
        std::shared_ptr<network::TCPClient> client;
        std::shared_ptr<ui::ConsoleUI> consoleUI;
    };

    std::unique_ptr<PeerSetup> createPeer(unsigned short port,
                                          const std::vector<std::string>& trustedPeers = {})
    {
        auto setup = std::make_unique<PeerSetup>();
        setup->port = port;
        setup->keyPair = crypto->generateKeyPair();

        // Create config
        setup->configPath = env->createTempFile("config_" + std::to_string(port) + ".json");
        nlohmann::json jConfig;
        jConfig["host"] = "127.0.0.1";
        jConfig["port"] = std::to_string(port);
        jConfig["private_key"] = crypto->keyToString(setup->keyPair.privateKey);
        jConfig["public_key"] = crypto->keyToString(setup->keyPair.publicKey);
        jConfig["trustedPeerList"] = nlohmann::json::array();
        for (const auto& peer : trustedPeers)
        {
            jConfig["trustedPeerList"].push_back(peer);
        }
        test_helpers::writeFile(setup->configPath, jConfig.dump(4));

        setup->config = std::make_shared<config::JsonConfig>(setup->configPath, crypto);

        // Create database
        setup->dbPath = env->createTestDatabase("peer_" + std::to_string(port));
        setup->db = std::make_shared<db::DBFile>(setup->dbPath);

        // Initialize repositories
        setup->peerRepo = std::make_shared<peer::PeerDB>(setup->db);
        setup->peerRepo->init();

        setup->chainRepo = std::make_shared<blockchain::ChainDB>(setup->db, setup->config, crypto);
        setup->chainRepo->init();

        setup->messageRepo = std::make_shared<message::MessageDB>(setup->db, setup->config, crypto);
        setup->messageRepo->init();

        // Initialize services
        std::vector<std::string> hosts = trustedPeers;
        setup->peerService = std::make_shared<peer::PeerService>(hosts, setup->peerRepo);

        setup->consoleUI = std::make_shared<ui::ConsoleUI>();

        setup->blockchainService = std::make_shared<blockchain::BlockchainService>(
            setup->config, crypto, setup->chainRepo, setup->consoleUI);

        setup->messageService = std::make_shared<message::MessageService>(
            setup->messageRepo, setup->blockchainService, setup->config, crypto, setup->consoleUI);

        setup->chatService = std::make_shared<chat::ChatService>(setup->config,
                                                                 crypto,
                                                                 setup->peerService,
                                                                 setup->blockchainService,
                                                                 setup->messageService,
                                                                 setup->consoleUI);

        // Initialize network
        setup->server = std::make_shared<network::TCPServer>(
            port, setup->chatService, setup->blockchainService, setup->consoleUI);

        blockchain::Block tip;
        setup->chainRepo->findTip(tip);

        setup->client = std::make_shared<network::TCPClient>(setup->config,
                                                             crypto,
                                                             setup->chatService,
                                                             setup->peerService,
                                                             setup->blockchainService,
                                                             setup->messageService,
                                                             setup->consoleUI,
                                                             tip.hash);

        return setup;
    }
};

TEST_F(FullChatScenarioTest, TwoPeersCommunicateSuccessfully)
{
    // Create two peers
    auto peer1 = createPeer(test_helpers::TEST_PORT_PEER1,
                            { "127.0.0.1:" + std::to_string(test_helpers::TEST_PORT_PEER2) });
    auto peer2 = createPeer(test_helpers::TEST_PORT_PEER2);

    // Start servers
    std::thread server1Thread([&peer1]() { peer1->server->start(); });
    std::thread server2Thread([&peer2]() { peer2->server->start(); });

    // Wait for servers to start
    ASSERT_TRUE(test_helpers::waitForPort(test_helpers::TEST_PORT_PEER1, 5000));
    ASSERT_TRUE(test_helpers::waitForPort(test_helpers::TEST_PORT_PEER2, 5000));

    // Peer1 connects to Peer2
    peer::UserPeer fromPeer1(
        "127.0.0.1", test_helpers::TEST_PORT_PEER1, crypto->keyToString(peer1->keyPair.publicKey));
    peer::UserPeer toPeer2(
        "127.0.0.1", test_helpers::TEST_PORT_PEER2, crypto->keyToString(peer2->keyPair.publicKey));

    message::ConnectionMessage connMsg(
        utils::uuidv4(), fromPeer1, toPeer2, utils::getTimestamp(), "0");

    EXPECT_NO_THROW(peer1->client->sendMessage(connMsg));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify connection was established
    auto peer1Peers = peer1->peerService->getPeers();
    EXPECT_GE(peer1Peers.size(), 1);

    auto peer2Peers = peer2->peerService->getPeers();
    ASSERT_GE(peer2Peers.size(), 1);
    EXPECT_EQ(peer2Peers[0].publicKey, crypto->keyToString(peer1->keyPair.publicKey));

    // Send a message from Peer1 to Peer2
    std::string messageContent = "Hello from Peer1!";
    message::TextMessage textMsg(
        utils::uuidv4(), fromPeer1, toPeer2, utils::getTimestamp(), messageContent, "0");

    EXPECT_NO_THROW(peer1->client->sendSecretMessage(textMsg));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Verify message was received and stored on Peer2
    std::vector<message::TextMessage> peer2Messages;
    peer2->messageService->findChatMessages(crypto->keyToString(peer2->keyPair.publicKey),
                                            crypto->keyToString(peer1->keyPair.publicKey),
                                            peer2Messages);

    ASSERT_GE(peer2Messages.size(), 1);
    EXPECT_EQ(peer2Messages[0].getPayload().message, messageContent);

    // Cleanup
    peer1->server->stop();
    peer2->server->stop();

    if (server1Thread.joinable()) server1Thread.join();
    if (server2Thread.joinable()) server2Thread.join();

    peer1->db->close();
    peer2->db->close();
}

TEST_F(FullChatScenarioTest, ThreePeersFormNetwork)
{
    // Create three peers with trust relationships
    auto peer1 = createPeer(test_helpers::TEST_PORT_PEER1,
                            { "127.0.0.1:" + std::to_string(test_helpers::TEST_PORT_PEER2) });
    auto peer2 = createPeer(test_helpers::TEST_PORT_PEER2,
                            { "127.0.0.1:" + std::to_string(test_helpers::TEST_PORT_PEER3) });
    auto peer3 = createPeer(test_helpers::TEST_PORT_PEER3);

    // Start servers
    std::thread server1Thread([&peer1]() { peer1->server->start(); });
    std::thread server2Thread([&peer2]() { peer2->server->start(); });
    std::thread server3Thread([&peer3]() { peer3->server->start(); });

    // Wait for servers to start
    ASSERT_TRUE(test_helpers::waitForPort(test_helpers::TEST_PORT_PEER1, 5000));
    ASSERT_TRUE(test_helpers::waitForPort(test_helpers::TEST_PORT_PEER2, 5000));
    ASSERT_TRUE(test_helpers::waitForPort(test_helpers::TEST_PORT_PEER3, 5000));

    // Establish connections
    peer::UserPeer p1(
        "127.0.0.1", test_helpers::TEST_PORT_PEER1, crypto->keyToString(peer1->keyPair.publicKey));
    peer::UserPeer p2(
        "127.0.0.1", test_helpers::TEST_PORT_PEER2, crypto->keyToString(peer2->keyPair.publicKey));
    peer::UserPeer p3(
        "127.0.0.1", test_helpers::TEST_PORT_PEER3, crypto->keyToString(peer3->keyPair.publicKey));

    // Peer1 -> Peer2
    message::ConnectionMessage conn1to2(utils::uuidv4(), p1, p2, utils::getTimestamp(), "0");
    EXPECT_NO_THROW(peer1->client->sendMessage(conn1to2));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Peer2 -> Peer3
    message::ConnectionMessage conn2to3(utils::uuidv4(), p2, p3, utils::getTimestamp(), "0");
    EXPECT_NO_THROW(peer2->client->sendMessage(conn2to3));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify network formation
    EXPECT_GE(peer1->peerService->getPeersCount(), 1);
    EXPECT_GE(peer2->peerService->getPeersCount(), 1);
    EXPECT_GE(peer3->peerService->getPeersCount(), 1);

    // Cleanup
    peer1->server->stop();
    peer2->server->stop();
    peer3->server->stop();

    if (server1Thread.joinable()) server1Thread.join();
    if (server2Thread.joinable()) server2Thread.join();
    if (server3Thread.joinable()) server3Thread.join();

    peer1->db->close();
    peer2->db->close();
    peer3->db->close();
}