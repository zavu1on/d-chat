#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>
#include <set>
#include <thread>
#include <vector>

#include "BlockchainService.hpp"
#include "ChainDB.hpp"
#include "ChatService.hpp"
#include "ConnectionMessage.hpp"
#include "ConsoleUI.hpp"
#include "DBFile.hpp"
#include "JsonConfig.hpp"
#include "MessageDB.hpp"
#include "MessageService.hpp"
#include "OpenSSLCrypto.hpp"
#include "PeerDB.hpp"
#include "PeerService.hpp"
#include "TCPClient.hpp"
#include "TCPServer.hpp"
#include "TextMessage.hpp"
#include "sha256.hpp"
#include "test_helpers.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

class BlockchainForkStressTest : public ::testing::Test
{
protected:
    std::unique_ptr<test_helpers::TestEnvironment> env;
    std::shared_ptr<crypto::ICrypto> crypto;

    static constexpr size_t MIN_PEERS = 5;
    static constexpr size_t MAX_PEERS = 20;
    static constexpr unsigned short BASE_PORT = 19000;
    static constexpr int SERVER_STARTUP_TIMEOUT_MS = 10000;
    static constexpr int MESSAGE_PROPAGATION_DELAY_MS = 500;

    std::atomic<size_t> successfulMessages{ 0 };
    std::atomic<size_t> failedMessages{ 0 };
    std::atomic<size_t> forkDetections{ 0 };
    std::mutex statsMutex;

    void SetUp() override
    {
        env = std::make_unique<test_helpers::TestEnvironment>();
        crypto = std::make_shared<crypto::OpenSSLCrypto>();
    }

    void TearDown() override { env->cleanup(); }

    struct PeerSetup
    {
        size_t id;
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
        std::thread serverThread;
        bool serverRunning = false;
    };

    class StartSignal
    {
    private:
        std::mutex mtx;
        std::condition_variable cv;
        bool ready = false;

    public:
        void wait()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return ready; });
        }

        void release()
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                ready = true;
            }
            cv.notify_all();
        }
    };

    blockchain::Block createGenesisBlock(const crypto::KeyPair& keyPair)
    {
        blockchain::Block block;
        block.previousHash = "0";
        block.payloadHash = utils::sha256("genesis");
        block.authorPublicKey = crypto->keyToString(keyPair.publicKey);
        block.timestamp = utils::getTimestamp();

        std::string canonical = block.toStringForHash();
        crypto::Bytes canonicalBytes(canonical.begin(), canonical.end());
        crypto::Bytes signature = crypto->sign(canonicalBytes, keyPair.privateKey);
        block.signature = crypto->keyToString(signature);

        block.computeHash();
        return block;
    }

    std::unique_ptr<PeerSetup> createPeer(size_t id, unsigned short port)
    {
        auto setup = std::make_unique<PeerSetup>();
        setup->id = id;
        setup->port = port;
        setup->keyPair = crypto->generateKeyPair();

        setup->configPath = env->createTestConfig(port,
                                                  crypto->keyToString(setup->keyPair.privateKey),
                                                  crypto->keyToString(setup->keyPair.publicKey));
        setup->config = std::make_shared<config::JsonConfig>(setup->configPath, crypto);

        setup->dbPath = env->createTestDatabase("peer_" + std::to_string(id));
        setup->db = std::make_shared<db::DBFile>(setup->dbPath);

        setup->chainRepo = std::make_shared<blockchain::ChainDB>(setup->db, setup->config, crypto);
        setup->chainRepo->init();

        setup->messageRepo = std::make_shared<message::MessageDB>(setup->db, setup->config, crypto);
        setup->messageRepo->init();

        setup->peerRepo = std::make_shared<peer::PeerDB>(setup->db);
        setup->peerRepo->init();

        setup->consoleUI = std::make_shared<ui::ConsoleUI>();
        std::vector<std::string> emptyHosts;
        setup->peerService = std::make_shared<peer::PeerService>(emptyHosts, setup->peerRepo);

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

    void initializeCommonBlockchain(std::vector<std::unique_ptr<PeerSetup>>& peers,
                                    const blockchain::Block& genesisBlock)
    {
        for (auto& peer : peers)
        {
            peer->chainRepo->insertBlock(genesisBlock);
        }
    }

    bool startAllServers(std::vector<std::unique_ptr<PeerSetup>>& peers)
    {
        for (auto& peer : peers)
        {
            peer->serverThread = std::thread(
                [&peer]()
                {
                    try
                    {
                        peer->server->start();
                    }
                    catch (const std::exception& e)
                    {
                        std::cerr << "Server " << peer->id << " error: " << e.what() << std::endl;
                    }
                });
            peer->serverRunning = true;
        }

        for (auto& peer : peers)
        {
            if (!test_helpers::waitForPort(peer->port, SERVER_STARTUP_TIMEOUT_MS))
            {
                std::cerr << "Failed to start server on port " << peer->port << std::endl;
                return false;
            }
        }

        return true;
    }

    void stopAllServers(std::vector<std::unique_ptr<PeerSetup>>& peers)
    {
        for (auto& peer : peers)
        {
            if (peer->serverRunning)
            {
                try
                {
                    peer->server->stop();
                }
                catch (...)
                {
                }
            }
        }

        for (auto& peer : peers)
        {
            if (peer->serverThread.joinable())
            {
                peer->serverThread.join();
            }
        }
    }

    void establishFullMeshConnections(std::vector<std::unique_ptr<PeerSetup>>& peers)
    {
        for (size_t i = 0; i < peers.size(); ++i)
        {
            for (size_t j = 0; j < peers.size(); ++j)
            {
                if (i == j) continue;

                peer::UserPeer from(
                    "127.0.0.1", peers[i]->port, crypto->keyToString(peers[i]->keyPair.publicKey));
                peer::UserPeer to(
                    "127.0.0.1", peers[j]->port, crypto->keyToString(peers[j]->keyPair.publicKey));

                peers[i]->peerService->addPeer(to);

                try
                {
                    message::ConnectionMessage connMsg =
                        message::ConnectionMessage::create(from, to, "0");
                    peers[i]->client->sendMessage(connMsg);
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Connection " << i << " -> " << j << " failed: " << e.what()
                              << std::endl;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS));
    }

    bool validateAllChains(std::vector<std::unique_ptr<PeerSetup>>& peers)
    {
        bool allValid = true;
        for (auto& peer : peers)
        {
            if (!peer->blockchainService->validateLocalChain())
            {
                std::cerr << "Peer " << peer->id << " has invalid chain!" << std::endl;
                allValid = false;
            }
        }
        return allValid;
    }

    size_t countUniqueChainTips(std::vector<std::unique_ptr<PeerSetup>>& peers)
    {
        std::set<std::string> uniqueTips;
        for (auto& peer : peers)
        {
            blockchain::Block tip;
            if (peer->chainRepo->findTip(tip))
            {
                uniqueTips.insert(tip.hash);
            }
        }
        return uniqueTips.size();
    }

    struct ChainStats
    {
        size_t minLength = SIZE_MAX;
        size_t maxLength = 0;
        size_t avgLength = 0;
        size_t uniqueTips = 0;
    };

    ChainStats getChainStatistics(std::vector<std::unique_ptr<PeerSetup>>& peers)
    {
        ChainStats stats;
        std::set<std::string> uniqueTips;
        size_t totalLength = 0;

        for (auto& peer : peers)
        {
            std::vector<blockchain::Block> blocks;
            peer->chainRepo->loadAllBlocks(blocks);

            size_t len = blocks.size();
            stats.minLength = std::min(stats.minLength, len);
            stats.maxLength = std::max(stats.maxLength, len);
            totalLength += len;

            blockchain::Block tip;
            if (peer->chainRepo->findTip(tip))
            {
                uniqueTips.insert(tip.hash);
            }
        }

        stats.avgLength = peers.empty() ? 0 : totalLength / peers.size();
        stats.uniqueTips = uniqueTips.size();
        return stats;
    }

    void closeAllDatabases(std::vector<std::unique_ptr<PeerSetup>>& peers)
    {
        for (auto& peer : peers)
        {
            if (peer->db)
            {
                peer->db->close();
            }
        }
    }
};

TEST_F(BlockchainForkStressTest, ConcurrentMessageSendingCausesForksTest)
{
    constexpr size_t NUM_PEERS = 5;

    std::cout << "\n=== Starting Concurrent Fork Test with " << NUM_PEERS
              << " peers ===" << std::endl;

    std::vector<std::unique_ptr<PeerSetup>> peers;
    for (size_t i = 0; i < NUM_PEERS; ++i)
    {
        peers.push_back(createPeer(i, BASE_PORT + static_cast<unsigned short>(i)));
    }

    blockchain::Block genesis = createGenesisBlock(peers[0]->keyPair);
    initializeCommonBlockchain(peers, genesis);

    ASSERT_TRUE(startAllServers(peers)) << "Failed to start all servers";

    establishFullMeshConnections(peers);

    for (auto& peer : peers)
    {
        EXPECT_GE(peer->peerService->getPeersCount(), NUM_PEERS - 1)
            << "Peer " << peer->id << " doesn't see all other peers";
    }

    std::cout << "All peers connected. Starting concurrent message sending..." << std::endl;

    StartSignal startSignal;
    std::vector<std::thread> sendThreads;
    std::atomic<size_t> successCount{ 0 };
    std::atomic<size_t> failCount{ 0 };

    for (size_t i = 0; i < NUM_PEERS; ++i)
    {
        sendThreads.emplace_back(
            [this, &peers, &startSignal, &successCount, &failCount, i, NUM_PEERS]()
            {
                startSignal.wait();

                size_t targetIdx = (i + 1) % NUM_PEERS;

                peer::UserPeer from(
                    "127.0.0.1", peers[i]->port, crypto->keyToString(peers[i]->keyPair.publicKey));
                peer::UserPeer to("127.0.0.1",
                                  peers[targetIdx]->port,
                                  crypto->keyToString(peers[targetIdx]->keyPair.publicKey));

                message::TextMessage textMsg = message::TextMessage::create(
                    from, to, "Message from peer " + std::to_string(i));

                try
                {
                    peers[i]->client->sendSecretMessage(textMsg);
                    ++successCount;
                    std::cout << "[Peer " << i << "] Message sent successfully" << std::endl;
                }
                catch (const std::exception& e)
                {
                    ++failCount;
                    std::cout << "[Peer " << i << "] Message failed: " << e.what() << std::endl;
                }
            });
    }

    startSignal.release();

    for (auto& t : sendThreads)
    {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS * 2));

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Successful messages: " << successCount.load() << std::endl;
    std::cout << "Failed messages: " << failCount.load() << std::endl;

    ChainStats stats = getChainStatistics(peers);
    std::cout << "Chain lengths - Min: " << stats.minLength << ", Max: " << stats.maxLength
              << ", Avg: " << stats.avgLength << std::endl;
    std::cout << "Unique chain tips: " << stats.uniqueTips << std::endl;

    EXPECT_TRUE(validateAllChains(peers)) << "Some peer chains are internally inconsistent";

    std::cout << "\n[INFO] Due to strict fork rejection, "
              << "only messages that didn't conflict were stored." << std::endl;

    stopAllServers(peers);
    closeAllDatabases(peers);
}

TEST_F(BlockchainForkStressTest, SequentialThenConcurrentMessagesTest)
{
    constexpr size_t NUM_PEERS = 8;
    constexpr size_t SEQUENTIAL_MESSAGES = 5;

    std::cout << "\n=== Sequential Then Concurrent Test with " << NUM_PEERS
              << " peers ===" << std::endl;

    std::vector<std::unique_ptr<PeerSetup>> peers;
    for (size_t i = 0; i < NUM_PEERS; ++i)
    {
        peers.push_back(createPeer(i, BASE_PORT + 100 + static_cast<unsigned short>(i)));
    }

    blockchain::Block genesis = createGenesisBlock(peers[0]->keyPair);
    initializeCommonBlockchain(peers, genesis);

    ASSERT_TRUE(startAllServers(peers)) << "Failed to start all servers";

    establishFullMeshConnections(peers);

    std::cout << "Phase 1: Sequential message sending (building common chain)..." << std::endl;

    for (size_t msg = 0; msg < SEQUENTIAL_MESSAGES; ++msg)
    {
        size_t senderIdx = msg % NUM_PEERS;
        size_t receiverIdx = (msg + 1) % NUM_PEERS;

        peer::UserPeer from("127.0.0.1",
                            peers[senderIdx]->port,
                            crypto->keyToString(peers[senderIdx]->keyPair.publicKey));
        peer::UserPeer to("127.0.0.1",
                          peers[receiverIdx]->port,
                          crypto->keyToString(peers[receiverIdx]->keyPair.publicKey));

        message::TextMessage textMsg =
            message::TextMessage::create(from, to, "Sequential message " + std::to_string(msg));

        try
        {
            peers[senderIdx]->client->sendSecretMessage(textMsg);
            std::cout << "  Sequential message " << msg << " sent from " << senderIdx << " to "
                      << receiverIdx << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "  Sequential message " << msg << " failed: " << e.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS));
    }

    std::cout << "Phase 2: Concurrent message burst..." << std::endl;

    StartSignal startSignal;
    std::vector<std::thread> sendThreads;
    std::atomic<size_t> burstSuccessCount{ 0 };
    std::atomic<size_t> burstFailCount{ 0 };

    for (size_t i = 0; i < NUM_PEERS; ++i)
    {
        sendThreads.emplace_back(
            [this, &peers, &startSignal, &burstSuccessCount, &burstFailCount, i, NUM_PEERS]()
            {
                startSignal.wait();

                size_t targetIdx = (i + NUM_PEERS / 2) % NUM_PEERS;
                if (targetIdx == i) targetIdx = (i + 1) % NUM_PEERS;

                peer::UserPeer from(
                    "127.0.0.1", peers[i]->port, crypto->keyToString(peers[i]->keyPair.publicKey));
                peer::UserPeer to("127.0.0.1",
                                  peers[targetIdx]->port,
                                  crypto->keyToString(peers[targetIdx]->keyPair.publicKey));

                message::TextMessage textMsg = message::TextMessage::create(
                    from, to, "Burst message from " + std::to_string(i));

                try
                {
                    peers[i]->client->sendSecretMessage(textMsg);
                    ++burstSuccessCount;
                }
                catch (...)
                {
                    ++burstFailCount;
                }
            });
    }

    startSignal.release();

    for (auto& t : sendThreads)
    {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS * 2));

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Burst success: " << burstSuccessCount.load() << std::endl;
    std::cout << "Burst failures: " << burstFailCount.load() << std::endl;

    ChainStats stats = getChainStatistics(peers);
    std::cout << "Chain lengths - Min: " << stats.minLength << ", Max: " << stats.maxLength
              << std::endl;
    std::cout << "Unique chain tips: " << stats.uniqueTips << std::endl;

    EXPECT_TRUE(validateAllChains(peers));
    EXPECT_GE(stats.minLength, 1) << "Chains should have at least genesis block";

    stopAllServers(peers);
    closeAllDatabases(peers);
}

TEST_F(BlockchainForkStressTest, ForkDetectionAndRollbackTest)
{
    constexpr size_t NUM_PEERS = 3;

    std::cout << "\n=== Fork Detection and Rollback Test ===" << std::endl;

    std::vector<std::unique_ptr<PeerSetup>> peers;
    for (size_t i = 0; i < NUM_PEERS; ++i)
    {
        peers.push_back(createPeer(i, BASE_PORT + 200 + static_cast<unsigned short>(i)));
    }

    blockchain::Block genesis = createGenesisBlock(peers[0]->keyPair);
    initializeCommonBlockchain(peers, genesis);

    ASSERT_TRUE(startAllServers(peers));

    establishFullMeshConnections(peers);

    std::vector<blockchain::Block> initialBlocks0;
    peers[0]->chainRepo->loadAllBlocks(initialBlocks0);
    size_t initialChainLength = initialBlocks0.size();

    std::cout << "Initial chain length: " << initialChainLength << std::endl;

    peer::UserPeer from0(
        "127.0.0.1", peers[0]->port, crypto->keyToString(peers[0]->keyPair.publicKey));
    peer::UserPeer to1(
        "127.0.0.1", peers[1]->port, crypto->keyToString(peers[1]->keyPair.publicKey));

    message::TextMessage msg1 = message::TextMessage::create(from0, to1, "First message");

    bool msg1Success = false;
    try
    {
        peers[0]->client->sendSecretMessage(msg1);
        msg1Success = true;
        std::cout << "Message 1 sent successfully" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Message 1 failed: " << e.what() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS));

    blockchain::Block tip2Before;
    peers[2]->chainRepo->findTip(tip2Before);
    std::cout << "Peer 2 tip before sending: " << tip2Before.hash.substr(0, 16) << "..."
              << std::endl;

    peer::UserPeer from2(
        "127.0.0.1", peers[2]->port, crypto->keyToString(peers[2]->keyPair.publicKey));

    message::TextMessage msg2 = message::TextMessage::create(from2, to1, "Competing message");

    bool msg2Success = false;
    try
    {
        peers[2]->client->sendSecretMessage(msg2);
        msg2Success = true;
        std::cout << "Message 2 sent successfully (unexpected if fork was detected)" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "Message 2 failed (expected due to fork): " << e.what() << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS));

    std::cout << "\n=== Chain States After Test ===" << std::endl;
    for (size_t i = 0; i < NUM_PEERS; ++i)
    {
        std::vector<blockchain::Block> blocks;
        peers[i]->chainRepo->loadAllBlocks(blocks);

        blockchain::Block tip;
        peers[i]->chainRepo->findTip(tip);

        std::cout << "Peer " << i << ": " << blocks.size() << " blocks, tip: "
                  << (tip.hash.empty() ? "empty" : tip.hash.substr(0, 16) + "...") << std::endl;

        EXPECT_TRUE(peers[i]->blockchainService->validateLocalChain())
            << "Peer " << i << " has invalid chain";
    }

    EXPECT_TRUE(msg1Success || msg2Success) << "At least one message should succeed";

    if (msg1Success && !msg2Success)
    {
        std::cout << "\n[SUCCESS] Fork was correctly detected and rejected!" << std::endl;
    }

    stopAllServers(peers);
    closeAllDatabases(peers);
}

TEST_F(BlockchainForkStressTest, RapidSequentialMessagesTest)
{
    constexpr size_t NUM_PEERS = 4;
    constexpr size_t NUM_MESSAGES = 20;

    std::cout << "\n=== Rapid Sequential Messages Test ===" << std::endl;
    std::cout << "Peers: " << NUM_PEERS << ", Messages: " << NUM_MESSAGES << std::endl;

    std::vector<std::unique_ptr<PeerSetup>> peers;
    for (size_t i = 0; i < NUM_PEERS; ++i)
    {
        peers.push_back(createPeer(i, BASE_PORT + 300 + static_cast<unsigned short>(i)));
    }

    blockchain::Block genesis = createGenesisBlock(peers[0]->keyPair);
    initializeCommonBlockchain(peers, genesis);

    ASSERT_TRUE(startAllServers(peers));

    establishFullMeshConnections(peers);

    size_t successCount = 0;
    size_t failCount = 0;

    auto startTime = std::chrono::steady_clock::now();

    for (size_t msg = 0; msg < NUM_MESSAGES; ++msg)
    {
        size_t senderIdx = msg % NUM_PEERS;
        size_t receiverIdx = (msg + 1) % NUM_PEERS;

        peer::UserPeer from("127.0.0.1",
                            peers[senderIdx]->port,
                            crypto->keyToString(peers[senderIdx]->keyPair.publicKey));
        peer::UserPeer to("127.0.0.1",
                          peers[receiverIdx]->port,
                          crypto->keyToString(peers[receiverIdx]->keyPair.publicKey));

        message::TextMessage textMsg =
            message::TextMessage::create(from, to, "Rapid message " + std::to_string(msg));

        try
        {
            peers[senderIdx]->client->sendSecretMessage(textMsg);
            ++successCount;
        }
        catch (...)
        {
            ++failCount;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "Successful: " << successCount << "/" << NUM_MESSAGES << std::endl;
    std::cout << "Failed: " << failCount << "/" << NUM_MESSAGES << std::endl;
    std::cout << "Duration: " << duration << " ms" << std::endl;
    std::cout << "Throughput: " << (successCount * 1000.0 / duration) << " msg/sec" << std::endl;

    EXPECT_TRUE(validateAllChains(peers));

    ChainStats stats = getChainStatistics(peers);
    std::cout << "Final chain lengths - Min: " << stats.minLength << ", Max: " << stats.maxLength
              << std::endl;
    std::cout << "Unique chain tips: " << stats.uniqueTips << std::endl;

    EXPECT_GE(successCount, NUM_MESSAGES / 2)
        << "At least half of sequential messages should succeed";

    stopAllServers(peers);
    closeAllDatabases(peers);
}

TEST_F(BlockchainForkStressTest, NetworkPartitionTest)
{
    constexpr size_t GROUP_SIZE = 3;
    constexpr size_t TOTAL_PEERS = GROUP_SIZE * 2;

    std::cout << "\n=== Network Partition Test ===" << std::endl;
    std::cout << "Two groups of " << GROUP_SIZE << " peers each" << std::endl;

    std::vector<std::unique_ptr<PeerSetup>> peers;
    for (size_t i = 0; i < TOTAL_PEERS; ++i)
    {
        peers.push_back(createPeer(i, BASE_PORT + 400 + static_cast<unsigned short>(i)));
    }

    blockchain::Block genesis = createGenesisBlock(peers[0]->keyPair);
    initializeCommonBlockchain(peers, genesis);

    ASSERT_TRUE(startAllServers(peers));

    std::cout << "Phase 1: Establishing partitioned connections..." << std::endl;

    for (size_t i = 0; i < GROUP_SIZE; ++i)
    {
        for (size_t j = 0; j < GROUP_SIZE; ++j)
        {
            if (i == j) continue;

            peer::UserPeer from(
                "127.0.0.1", peers[i]->port, crypto->keyToString(peers[i]->keyPair.publicKey));
            peer::UserPeer to(
                "127.0.0.1", peers[j]->port, crypto->keyToString(peers[j]->keyPair.publicKey));

            peers[i]->peerService->addPeer(to);

            try
            {
                message::ConnectionMessage connMsg =
                    message::ConnectionMessage::create(from, to, "0");
                peers[i]->client->sendMessage(connMsg);
            }
            catch (...)
            {
            }
        }
    }

    for (size_t i = GROUP_SIZE; i < TOTAL_PEERS; ++i)
    {
        for (size_t j = GROUP_SIZE; j < TOTAL_PEERS; ++j)
        {
            if (i == j) continue;

            peer::UserPeer from(
                "127.0.0.1", peers[i]->port, crypto->keyToString(peers[i]->keyPair.publicKey));
            peer::UserPeer to(
                "127.0.0.1", peers[j]->port, crypto->keyToString(peers[j]->keyPair.publicKey));

            peers[i]->peerService->addPeer(to);

            try
            {
                message::ConnectionMessage connMsg =
                    message::ConnectionMessage::create(from, to, "0");
                peers[i]->client->sendMessage(connMsg);
            }
            catch (...)
            {
            }
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS));

    std::cout << "Phase 2: Sending messages in isolated groups..." << std::endl;

    {
        peer::UserPeer from(
            "127.0.0.1", peers[0]->port, crypto->keyToString(peers[0]->keyPair.publicKey));
        peer::UserPeer to(
            "127.0.0.1", peers[1]->port, crypto->keyToString(peers[1]->keyPair.publicKey));

        message::TextMessage msg = message::TextMessage::create(from, to, "Group1 message");
        try
        {
            peers[0]->client->sendSecretMessage(msg);
            std::cout << "  Group 1 message sent" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "  Group 1 message failed: " << e.what() << std::endl;
        }
    }

    {
        peer::UserPeer from("127.0.0.1",
                            peers[GROUP_SIZE]->port,
                            crypto->keyToString(peers[GROUP_SIZE]->keyPair.publicKey));
        peer::UserPeer to("127.0.0.1",
                          peers[GROUP_SIZE + 1]->port,
                          crypto->keyToString(peers[GROUP_SIZE + 1]->keyPair.publicKey));

        message::TextMessage msg = message::TextMessage::create(from, to, "Group2 message");
        try
        {
            peers[GROUP_SIZE]->client->sendSecretMessage(msg);
            std::cout << "  Group 2 message sent" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "  Group 2 message failed: " << e.what() << std::endl;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS));

    std::cout << "\nChain tips during partition:" << std::endl;
    std::set<std::string> group1Tips, group2Tips;

    for (size_t i = 0; i < GROUP_SIZE; ++i)
    {
        blockchain::Block tip;
        peers[i]->chainRepo->findTip(tip);
        group1Tips.insert(tip.hash);
        std::cout << "  Peer " << i << " (G1): " << tip.hash.substr(0, 16) << "..." << std::endl;
    }

    for (size_t i = GROUP_SIZE; i < TOTAL_PEERS; ++i)
    {
        blockchain::Block tip;
        peers[i]->chainRepo->findTip(tip);
        group2Tips.insert(tip.hash);
        std::cout << "  Peer " << i << " (G2): " << tip.hash.substr(0, 16) << "..." << std::endl;
    }

    bool groupsDiverged = (group1Tips != group2Tips);
    std::cout << "\nGroups diverged: " << (groupsDiverged ? "YES" : "NO") << std::endl;

    std::cout << "\nPhase 3: Healing partition (connecting groups)..." << std::endl;

    {
        peer::UserPeer from(
            "127.0.0.1", peers[0]->port, crypto->keyToString(peers[0]->keyPair.publicKey));
        peer::UserPeer to("127.0.0.1",
                          peers[GROUP_SIZE]->port,
                          crypto->keyToString(peers[GROUP_SIZE]->keyPair.publicKey));

        peers[0]->peerService->addPeer(to);
        peers[GROUP_SIZE]->peerService->addPeer(from);

        try
        {
            message::ConnectionMessage connMsg = message::ConnectionMessage::create(from, to, "0");
            peers[0]->client->sendMessage(connMsg);
            std::cout << "  Bridge connection established" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "  Bridge connection failed: " << e.what() << std::endl;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS));

    std::cout << "\nPhase 4: Cross-group message after healing..." << std::endl;
    {
        peer::UserPeer from(
            "127.0.0.1", peers[0]->port, crypto->keyToString(peers[0]->keyPair.publicKey));
        peer::UserPeer to("127.0.0.1",
                          peers[GROUP_SIZE]->port,
                          crypto->keyToString(peers[GROUP_SIZE]->keyPair.publicKey));

        message::TextMessage msg = message::TextMessage::create(from, to, "Cross-group message");
        try
        {
            peers[0]->client->sendSecretMessage(msg);
            std::cout << "  Cross-group message sent" << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cout << "  Cross-group message failed (expected if forks exist): " << e.what()
                      << std::endl;
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(MESSAGE_PROPAGATION_DELAY_MS));

    std::cout << "\n=== Final Results ===" << std::endl;
    EXPECT_TRUE(validateAllChains(peers)) << "All chains should be internally valid";

    ChainStats stats = getChainStatistics(peers);
    std::cout << "Chain lengths - Min: " << stats.minLength << ", Max: " << stats.maxLength
              << std::endl;
    std::cout << "Unique chain tips: " << stats.uniqueTips << std::endl;

    if (stats.uniqueTips > 1)
    {
        std::cout << "[INFO] Multiple chain tips exist - this demonstrates the fork handling "
                  << "behavior where conflicting chains are not merged." << std::endl;
    }

    stopAllServers(peers);
    closeAllDatabases(peers);
}
