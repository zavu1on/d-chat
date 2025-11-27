#include <gtest/gtest.h>

#include "BlockchainService.hpp"
#include "ChainDB.hpp"
#include "ConsoleUI.hpp"
#include "DBFile.hpp"
#include "JsonConfig.hpp"
#include "MessageDB.hpp"
#include "MessageService.hpp"
#include "OpenSSLCrypto.hpp"
#include "test_helpers.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

class MessageServiceTest : public ::testing::Test
{
protected:
    std::unique_ptr<test_helpers::TestEnvironment> env;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<db::DBFile> db;
    std::shared_ptr<message::IMessageRepo> messageRepo;
    std::shared_ptr<blockchain::IChainRepo> chainRepo;
    std::shared_ptr<blockchain::BlockchainService> blockchainService;
    std::shared_ptr<message::MessageService> messageService;
    std::shared_ptr<ui::ConsoleUI> consoleUI;
    crypto::KeyPair keyPair1;
    crypto::KeyPair keyPair2;

    void SetUp() override
    {
        env = std::make_unique<test_helpers::TestEnvironment>();
        crypto = std::make_shared<crypto::OpenSSLCrypto>();
        keyPair1 = crypto->generateKeyPair();
        keyPair2 = crypto->generateKeyPair();

        std::string configPath = env->createTestConfig(test_helpers::TEST_PORT_BASE,
                                                       crypto->keyToString(keyPair1.privateKey),
                                                       crypto->keyToString(keyPair1.publicKey));

        config = std::make_shared<config::JsonConfig>(configPath, crypto);

        std::string dbPath = env->createTestDatabase("message_test");
        db = std::make_shared<db::DBFile>(dbPath);

        chainRepo = std::make_shared<blockchain::ChainDB>(db, config, crypto);
        chainRepo->init();

        messageRepo = std::make_shared<message::MessageDB>(db, config, crypto);
        messageRepo->init();

        consoleUI = std::make_shared<ui::ConsoleUI>();

        blockchainService =
            std::make_shared<blockchain::BlockchainService>(config, crypto, chainRepo, consoleUI);

        messageService = std::make_shared<message::MessageService>(
            messageRepo, chainRepo, blockchainService, config, crypto, consoleUI);
    }

    void TearDown() override
    {
        db->close();
        env->cleanup();
    }
};

TEST_F(MessageServiceTest, InsertAndFindChatMessagesSucceeds)
{
    peer::UserPeer peer1(
        "127.0.0.1", test_helpers::TEST_PORT_PEER1, crypto->keyToString(keyPair1.publicKey));
    peer::UserPeer peer2(
        "127.0.0.1", test_helpers::TEST_PORT_PEER2, crypto->keyToString(keyPair2.publicKey));

    message::TextMessage msg1(
        utils::uuidv4(), peer1, peer2, utils::getTimestamp(), "Hello from peer1", "0");

    nlohmann::json jData1;
    msg1.serialize(jData1, crypto->keyToString(keyPair1.privateKey), crypto);

    EXPECT_TRUE(messageService->insertSecretMessage(msg1, jData1.dump(), "blockhash1"));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    message::TextMessage msg2(
        utils::uuidv4(), peer2, peer1, utils::getTimestamp(), "Reply from peer2", "0");

    nlohmann::json jData2;
    msg2.serialize(jData2, crypto->keyToString(keyPair2.privateKey), crypto);

    EXPECT_TRUE(messageService->insertSecretMessage(msg2, jData2.dump(), "blockhash2"));

    std::vector<message::TextMessage> messages;
    messageService->findChatMessages(
        crypto->keyToString(keyPair1.publicKey), crypto->keyToString(keyPair2.publicKey), messages);

    ASSERT_EQ(messages.size(), 2);
    EXPECT_EQ(messages[0].getPayload().message, "Hello from peer1");
    EXPECT_EQ(messages[1].getPayload().message, "Reply from peer2");
}

TEST_F(MessageServiceTest, FindChatMessagesReturnsEmptyForNonExistentChat)
{
    std::vector<message::TextMessage> messages;
    messageService->findChatMessages("nonexistent1", "nonexistent2", messages);

    EXPECT_TRUE(messages.empty());
}

TEST_F(MessageServiceTest, FindInvalidChatMessageIDsDetectsInvalidMessages)
{
    peer::UserPeer peer1(
        "127.0.0.1", test_helpers::TEST_PORT_PEER1, crypto->keyToString(keyPair1.publicKey));
    peer::UserPeer peer2(
        "127.0.0.1", test_helpers::TEST_PORT_PEER2, crypto->keyToString(keyPair2.publicKey));

    message::TextMessage msg(
        utils::uuidv4(), peer1, peer2, utils::getTimestamp(), "Test message", "0");

    nlohmann::json jData;
    msg.serialize(jData, crypto->keyToString(keyPair1.privateKey), crypto);

    EXPECT_TRUE(messageService->insertSecretMessage(msg, jData.dump(), "nonexistent_block"));

    std::vector<message::TextMessage> messages;
    messageService->findChatMessages(
        crypto->keyToString(keyPair1.publicKey), crypto->keyToString(keyPair2.publicKey), messages);

    ASSERT_EQ(messages.size(), 1);

    std::vector<std::string> invalidIds;
    messageService->findInvalidChatMessageIDs(messages, invalidIds);

    ASSERT_EQ(invalidIds.size(), 1);
    EXPECT_EQ(invalidIds[0], msg.getId());
}

TEST_F(MessageServiceTest, RemoveMessageByBlockHashSucceeds)
{
    peer::UserPeer peer1(
        "127.0.0.1", test_helpers::TEST_PORT_PEER1, crypto->keyToString(keyPair1.publicKey));
    peer::UserPeer peer2(
        "127.0.0.1", test_helpers::TEST_PORT_PEER2, crypto->keyToString(keyPair2.publicKey));

    message::TextMessage msg(
        utils::uuidv4(), peer1, peer2, utils::getTimestamp(), "Message to be removed", "0");

    nlohmann::json jData;
    msg.serialize(jData, crypto->keyToString(keyPair1.privateKey), crypto);

    std::string blockHash = "test_block_hash";
    EXPECT_TRUE(messageService->insertSecretMessage(msg, jData.dump(), blockHash));

    std::vector<message::TextMessage> messages;
    messageService->findChatMessages(
        crypto->keyToString(keyPair1.publicKey), crypto->keyToString(keyPair2.publicKey), messages);
    EXPECT_EQ(messages.size(), 1);

    EXPECT_TRUE(messageService->removeMessageByBlockHash(blockHash));

    messages.clear();
    messageService->findChatMessages(
        crypto->keyToString(keyPair1.publicKey), crypto->keyToString(keyPair2.publicKey), messages);
    EXPECT_TRUE(messages.empty());
}