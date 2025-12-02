#include <gtest/gtest.h>

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
#include "PeerListMessage.hpp"
#include "PeerService.hpp"
#include "TextMessage.hpp"
#include "test_helpers.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

class ChatServiceTest : public ::testing::Test
{
protected:
    std::unique_ptr<test_helpers::TestEnvironment> env;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<db::DBFile> db;
    std::shared_ptr<peer::IPeerRepo> peerRepo;
    std::shared_ptr<blockchain::IChainRepo> chainRepo;
    std::shared_ptr<message::IMessageRepo> messageRepo;
    std::shared_ptr<peer::PeerService> peerService;
    std::shared_ptr<blockchain::BlockchainService> blockchainService;
    std::shared_ptr<message::MessageService> messageService;
    std::shared_ptr<chat::ChatService> chatService;
    std::shared_ptr<ui::ConsoleUI> consoleUI;
    crypto::KeyPair keyPair;

    void SetUp() override
    {
        env = std::make_unique<test_helpers::TestEnvironment>();
        crypto = std::make_shared<crypto::OpenSSLCrypto>();
        keyPair = crypto->generateKeyPair();

        std::string configPath = env->createTestConfig(test_helpers::TEST_PORT_BASE,
                                                       crypto->keyToString(keyPair.privateKey),
                                                       crypto->keyToString(keyPair.publicKey));

        config = std::make_shared<config::JsonConfig>(configPath, crypto);

        std::string dbPath = env->createTestDatabase("chat_service_test");
        db = std::make_shared<db::DBFile>(dbPath);

        peerRepo = std::make_shared<peer::PeerDB>(db);
        peerRepo->init();

        chainRepo = std::make_shared<blockchain::ChainDB>(db, config, crypto);
        chainRepo->init();

        messageRepo = std::make_shared<message::MessageDB>(db, config, crypto);
        messageRepo->init();

        std::vector<std::string> hosts;
        peerService = std::make_shared<peer::PeerService>(hosts, peerRepo);

        consoleUI = std::make_shared<ui::ConsoleUI>();

        blockchainService =
            std::make_shared<blockchain::BlockchainService>(config, crypto, chainRepo, consoleUI);

        messageService = std::make_shared<message::MessageService>(
            messageRepo, blockchainService, config, crypto, consoleUI);

        chatService = std::make_shared<chat::ChatService>(
            config, crypto, peerService, blockchainService, messageService, consoleUI);
    }

    void TearDown() override
    {
        db->close();
        env->cleanup();
    }
};

TEST_F(ChatServiceTest, HandleIncomingConnectionMessageSucceeds)
{
    peer::UserPeer from = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);
    peer::UserPeer to(
        "127.0.0.1", test_helpers::TEST_PORT_BASE, crypto->keyToString(keyPair.publicKey));

    message::ConnectionMessage connMsg(utils::uuidv4(), from, to, utils::getTimestamp(), "0");

    nlohmann::json jData;
    connMsg.serialize(jData);

    std::string response;
    chatService->handleIncomingMessage(jData, response);

    EXPECT_FALSE(response.empty());
    EXPECT_NE(response, "{}");

    nlohmann::json jResponse = nlohmann::json::parse(response);
    EXPECT_EQ(jResponse["type"].get<std::string>(), "CONNECT_RESPONSE");

    // Verify peer was added
    auto peers = peerService->getPeers();
    ASSERT_EQ(peers.size(), 1);
    EXPECT_EQ(peers[0].publicKey, from.publicKey);
}

TEST_F(ChatServiceTest, HandleIncomingPeerListMessageSucceeds)
{
    // Add some peers to service
    peer::UserPeer peer1 = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);
    peer::UserPeer peer2 = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER2, crypto);
    peerService->addPeer(peer1);
    peerService->addPeer(peer2);

    peer::UserPeer from = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER3, crypto);
    peer::UserPeer to(
        "127.0.0.1", test_helpers::TEST_PORT_BASE, crypto->keyToString(keyPair.publicKey));

    message::PeerListMessage peerListMsg(utils::uuidv4(), from, to, utils::getTimestamp(), 0, 10);

    nlohmann::json jData;
    peerListMsg.serialize(jData);

    std::string response;
    chatService->handleIncomingMessage(jData, response);

    EXPECT_FALSE(response.empty());

    nlohmann::json jResponse = nlohmann::json::parse(response);
    EXPECT_EQ(jResponse["type"].get<std::string>(), "PEER_LIST_RESPONSE");

    // Should return peers excluding the requestor
    auto payload = jResponse["payload"]["peers"];
    EXPECT_TRUE(payload.is_array());
    EXPECT_LE(payload.size(), 2);
}

TEST_F(ChatServiceTest, HandleOutgoingConnectionMessageUpdatesPeerCount)
{
    peer::UserPeer from = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);
    peer::UserPeer to(
        "127.0.0.1", test_helpers::TEST_PORT_BASE, crypto->keyToString(keyPair.publicKey));

    message::ConnectionMessageResponse connResponse(utils::uuidv4(),
                                                    from,
                                                    to,
                                                    utils::getTimestamp(),
                                                    5,  // peersToReceive
                                                    10  // missingBlocksCount
    );

    nlohmann::json jData;
    connResponse.serialize(jData);

    chatService->handleOutgoingMessage(jData.dump());

    // Verify peer was added
    auto peers = peerService->getPeers();
    ASSERT_EQ(peers.size(), 1);
    EXPECT_EQ(peers[0].publicKey, from.publicKey);
}

TEST_F(ChatServiceTest, HandleIncomingTextMessageStoresMessage)
{
    auto keyPair2 = crypto->generateKeyPair();

    peer::UserPeer from(
        "127.0.0.1", test_helpers::TEST_PORT_PEER1, crypto->keyToString(keyPair2.publicKey));
    peer::UserPeer to(
        "127.0.0.1", test_helpers::TEST_PORT_BASE, crypto->keyToString(keyPair.publicKey));

    std::string messageContent = "Hello, this is a test message";

    message::TextMessage textMsg(
        utils::uuidv4(), from, to, utils::getTimestamp(), messageContent, "test_block_hash");

    nlohmann::json jData;
    textMsg.serialize(jData, crypto->keyToString(keyPair2.privateKey), crypto);

    std::string response;
    chatService->handleIncomingMessage(jData, response);

    EXPECT_FALSE(response.empty());

    nlohmann::json jResponse = nlohmann::json::parse(response);
    EXPECT_EQ(jResponse["type"].get<std::string>(), "TEXT_MESSAGE_RESPONSE");

    // Verify message was stored
    std::vector<message::TextMessage> messages;
    messageService->findChatMessages(
        crypto->keyToString(keyPair.publicKey), crypto->keyToString(keyPair2.publicKey), messages);

    ASSERT_EQ(messages.size(), 1);
    EXPECT_EQ(messages[0].getPayload().message, messageContent);
}

TEST_F(ChatServiceTest, HandleIncomingBlockRangeMessageReturnsBlocks)
{
    // Create and insert some test blocks
    blockchain::Block block1;
    block1.hash = "hash1";
    block1.previousHash = "0";
    block1.payloadHash = "payload1";
    block1.authorPublicKey = crypto->keyToString(keyPair.publicKey);
    block1.timestamp = utils::getTimestamp();
    block1.signature = "sig1";
    chainRepo->insertBlock(block1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    blockchain::Block block2;
    block2.hash = "hash2";
    block2.previousHash = "hash1";
    block2.payloadHash = "payload2";
    block2.authorPublicKey = crypto->keyToString(keyPair.publicKey);
    block2.timestamp = utils::getTimestamp();
    block2.signature = "sig2";
    chainRepo->insertBlock(block2);

    peer::UserPeer from = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);
    peer::UserPeer to(
        "127.0.0.1", test_helpers::TEST_PORT_BASE, crypto->keyToString(keyPair.publicKey));

    message::BlockRangeMessage blockRangeMsg(
        utils::uuidv4(), from, to, utils::getTimestamp(), 0, 2, "0");

    nlohmann::json jData;
    blockRangeMsg.serialize(jData);

    std::string response;
    chatService->handleIncomingMessage(jData, response);

    EXPECT_FALSE(response.empty());

    nlohmann::json jResponse = nlohmann::json::parse(response);
    EXPECT_EQ(jResponse["type"].get<std::string>(), "BLOCK_RANGE_RESPONSE");

    auto blocks = jResponse["payload"]["blocks"];
    EXPECT_TRUE(blocks.is_array());
    EXPECT_EQ(blocks.size(), 2);
}

TEST_F(ChatServiceTest, HandleIncomingInvalidMessageReturnsError)
{
    nlohmann::json invalidJson1;
    invalidJson1["type"] = "TEXT_MESSAGE";
    invalidJson1["id"] = utils::uuidv4();
    // Missing required fields: from, to, timestamp, payload

    std::string response1;
    chatService->handleIncomingMessage(invalidJson1, response1);

    EXPECT_FALSE(response1.empty());
    nlohmann::json jResponse1 = nlohmann::json::parse(response1);
    EXPECT_EQ(jResponse1["type"].get<std::string>(), "ERROR_RESPONSE");
    EXPECT_TRUE(jResponse1.contains("error") || jResponse1.contains("payload"));

    nlohmann::json invalidJson2;
    invalidJson2["id"] = utils::uuidv4();
    invalidJson2["from"] = nlohmann::json::object();
    invalidJson2["to"] = nlohmann::json::object();
    // Missing type field

    std::string response2;
    chatService->handleIncomingMessage(invalidJson2, response2);

    EXPECT_FALSE(response2.empty());
    nlohmann::json jResponse2 = nlohmann::json::parse(response2);
    EXPECT_EQ(jResponse2["type"].get<std::string>(), "ERROR_RESPONSE");

    nlohmann::json invalidJson3;
    invalidJson3["type"] = "UNKNOWN_MESSAGE_TYPE";
    invalidJson3["id"] = utils::uuidv4();
    invalidJson3["from"] = nlohmann::json::object(
        { { "host", "127.0.0.1" }, { "port", 8001 }, { "public_key", "test_key" } });
    invalidJson3["to"] = nlohmann::json::object(
        { { "host", "127.0.0.1" }, { "port", 8002 }, { "public_key", "test_key2" } });
    invalidJson3["timestamp"] = utils::getTimestamp();

    std::string response3;
    chatService->handleIncomingMessage(invalidJson3, response3);

    EXPECT_FALSE(response3.empty());
    nlohmann::json jResponse3 = nlohmann::json::parse(response3);
    EXPECT_EQ(jResponse3["type"].get<std::string>(), "ERROR_RESPONSE");
}

TEST_F(ChatServiceTest, HandleIncomingMalformedJSONReturnsError)
{
    nlohmann::json malformedJson;
    malformedJson["type"] = "TEXT_MESSAGE";
    malformedJson["id"] = nullptr;  // null instead of string
    malformedJson["from"] = nullptr;
    malformedJson["to"] = nullptr;

    std::string response;
    chatService->handleIncomingMessage(malformedJson, response);

    EXPECT_FALSE(response.empty());
    nlohmann::json jResponse = nlohmann::json::parse(response);
    EXPECT_EQ(jResponse["type"].get<std::string>(), "ERROR_RESPONSE");
}