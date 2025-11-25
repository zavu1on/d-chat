#include <gtest/gtest.h>

#include "BlockchainService.hpp"
#include "ChainDB.hpp"
#include "ConsoleUI.hpp"
#include "DBFile.hpp"
#include "JsonConfig.hpp"
#include "OpenSSLCrypto.hpp"
#include "TextMessage.hpp"
#include "sha256.hpp"
#include "test_helpers.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

class BlockchainServiceTest : public ::testing::Test
{
protected:
    std::unique_ptr<test_helpers::TestEnvironment> env;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<db::DBFile> db;
    std::shared_ptr<blockchain::IChainRepo> chainRepo;
    std::shared_ptr<blockchain::BlockchainService> blockchainService;
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

        std::string dbPath = env->createTestDatabase("blockchain_test");
        db = std::make_shared<db::DBFile>(dbPath);

        chainRepo = std::make_shared<blockchain::ChainDB>(db, config, crypto);
        chainRepo->init();

        consoleUI = std::make_shared<ui::ConsoleUI>();

        blockchainService =
            std::make_shared<blockchain::BlockchainService>(config, crypto, chainRepo, consoleUI);
    }

    void TearDown() override
    {
        db->close();
        env->cleanup();
    }

    blockchain::Block createValidBlock(const std::string& previousHash, const std::string& payload)
    {
        blockchain::Block block;
        block.previousHash = previousHash;
        block.payloadHash = utils::sha256(payload);
        block.authorPublicKey = crypto->keyToString(keyPair.publicKey);
        block.timestamp = utils::getTimestamp();

        std::string canonical = block.toStringForHash();
        crypto::Bytes canonicalBytes(canonical.begin(), canonical.end());
        crypto::Bytes signature = crypto->sign(canonicalBytes, keyPair.privateKey);
        block.signature = crypto->keyToString(signature);

        block.computeHash();
        return block;
    }
};

TEST_F(BlockchainServiceTest, ValidateLocalChainWithEmptyChainSucceeds)
{
    EXPECT_TRUE(blockchainService->validateLocalChain());
}

TEST_F(BlockchainServiceTest, ValidateLocalChainWithSingleBlockSucceeds)
{
    blockchain::Block block = createValidBlock("0", "genesis block");

    EXPECT_TRUE(chainRepo->insertBlock(block));
    EXPECT_TRUE(blockchainService->validateLocalChain());
}

TEST_F(BlockchainServiceTest, ValidateLocalChainWithMultipleBlocksSucceeds)
{
    blockchain::Block block1 = createValidBlock("0", "first block");
    EXPECT_TRUE(chainRepo->insertBlock(block1));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    blockchain::Block block2 = createValidBlock(block1.hash, "second block");
    EXPECT_TRUE(chainRepo->insertBlock(block2));

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    blockchain::Block block3 = createValidBlock(block2.hash, "third block");
    EXPECT_TRUE(chainRepo->insertBlock(block3));

    EXPECT_TRUE(blockchainService->validateLocalChain());
}

TEST_F(BlockchainServiceTest, ValidateSingleBlockSucceeds)
{
    blockchain::Block block = createValidBlock("0", "test block");

    std::string error;
    EXPECT_TRUE(blockchainService->validateIncomingBlock(block, error));
    EXPECT_TRUE(error.empty());
}

TEST_F(BlockchainServiceTest, ValidateBlockWithInvalidSignatureFails)
{
    blockchain::Block block = createValidBlock("0", "test block");
    block.signature = "invalid_signature";

    std::string error;
    EXPECT_FALSE(blockchainService->validateIncomingBlock(block, error));
    EXPECT_FALSE(error.empty());
}

TEST_F(BlockchainServiceTest, ValidateBlockWithInvalidHashFails)
{
    blockchain::Block block = createValidBlock("0", "test block");
    block.hash = "wrong_hash";

    std::string error;
    EXPECT_FALSE(blockchainService->validateIncomingBlock(block, error));
    EXPECT_FALSE(error.empty());
}

TEST_F(BlockchainServiceTest, ValidateBlockWithFutureTimestampFails)
{
    blockchain::Block block = createValidBlock("0", "test block");
    block.timestamp = utils::getTimestamp() + 400000;  // More than 5 minutes in future

    // Recompute hash and signature with new timestamp
    std::string canonical = block.toStringForHash();
    crypto::Bytes canonicalBytes(canonical.begin(), canonical.end());
    crypto::Bytes signature = crypto->sign(canonicalBytes, keyPair.privateKey);
    block.signature = crypto->keyToString(signature);
    block.computeHash();

    std::string error;
    EXPECT_FALSE(blockchainService->validateIncomingBlock(block, error));
    EXPECT_NE(error.find("future"), std::string::npos);
}

TEST_F(BlockchainServiceTest, CreateBlockFromMessageSucceeds)
{
    peer::UserPeer from = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);
    peer::UserPeer to = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER2, crypto);

    message::TextMessage textMsg(
        utils::uuidv4(), from, to, utils::getTimestamp(), "Hello, World!", "0");

    blockchain::Block block;
    blockchainService->createBlockFromMessage(textMsg, block);

    EXPECT_EQ(block.previousHash, "0");
    EXPECT_EQ(block.authorPublicKey, from.publicKey);
    EXPECT_FALSE(block.hash.empty());
    EXPECT_FALSE(block.signature.empty());
    EXPECT_EQ(block.payloadHash, utils::sha256("Hello, World!"));
}

TEST_F(BlockchainServiceTest, CompareBlockWithMessageSucceeds)
{
    peer::UserPeer from(
        "127.0.0.1", test_helpers::TEST_PORT_PEER1, crypto->keyToString(keyPair.publicKey));
    peer::UserPeer to = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER2, crypto);

    std::string messageContent = "Test message content";
    message::TextMessage textMsg(
        utils::uuidv4(), from, to, utils::getTimestamp(), messageContent, "0");

    blockchain::Block block;
    blockchainService->createBlockFromMessage(textMsg, block);

    std::string error;
    EXPECT_TRUE(blockchainService->compareBlockWithMessage(block, textMsg, error));
    EXPECT_TRUE(error.empty());
}

TEST_F(BlockchainServiceTest, CompareBlockWithDifferentMessageFails)
{
    peer::UserPeer from(
        "127.0.0.1", test_helpers::TEST_PORT_PEER1, crypto->keyToString(keyPair.publicKey));
    peer::UserPeer to = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER2, crypto);

    message::TextMessage textMsg1(
        utils::uuidv4(), from, to, utils::getTimestamp(), "First message", "0");

    message::TextMessage textMsg2(
        utils::uuidv4(), from, to, utils::getTimestamp(), "Different message", "0");

    blockchain::Block block;
    blockchainService->createBlockFromMessage(textMsg1, block);

    std::string error;
    EXPECT_FALSE(blockchainService->compareBlockWithMessage(block, textMsg2, error));
    EXPECT_FALSE(error.empty());
}

TEST_F(BlockchainServiceTest, CountBlocksAfterHashWorks)
{
    blockchain::Block block1 = createValidBlock("0", "block 1");
    chainRepo->insertBlock(block1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    blockchain::Block block2 = createValidBlock(block1.hash, "block 2");
    chainRepo->insertBlock(block2);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    blockchain::Block block3 = createValidBlock(block2.hash, "block 3");
    chainRepo->insertBlock(block3);

    EXPECT_EQ(blockchainService->countBlocksAfterHash("0"), 3);
    EXPECT_EQ(blockchainService->countBlocksAfterHash(block1.hash), 2);
    EXPECT_EQ(blockchainService->countBlocksAfterHash(block2.hash), 1);
    EXPECT_EQ(blockchainService->countBlocksAfterHash(block3.hash), 0);
}

TEST_F(BlockchainServiceTest, GetBlocksByIndexRangeWorks)
{
    std::vector<blockchain::Block> originalBlocks;

    blockchain::Block block1 = createValidBlock("0", "block 1");
    chainRepo->insertBlock(block1);
    originalBlocks.push_back(block1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    blockchain::Block block2 = createValidBlock(block1.hash, "block 2");
    chainRepo->insertBlock(block2);
    originalBlocks.push_back(block2);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    blockchain::Block block3 = createValidBlock(block2.hash, "block 3");
    chainRepo->insertBlock(block3);
    originalBlocks.push_back(block3);

    std::vector<blockchain::Block> retrieved;
    blockchainService->getBlocksByIndexRange(0, 2, "0", retrieved);

    ASSERT_EQ(retrieved.size(), 2);
    EXPECT_EQ(retrieved[0].hash, block1.hash);
    EXPECT_EQ(retrieved[1].hash, block2.hash);

    retrieved.clear();
    blockchainService->getBlocksByIndexRange(1, 2, "0", retrieved);

    ASSERT_EQ(retrieved.size(), 2);
    EXPECT_EQ(retrieved[0].hash, block2.hash);
    EXPECT_EQ(retrieved[1].hash, block3.hash);
}