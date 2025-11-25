#include <gtest/gtest.h>

#include "BlockRangeMessage.hpp"
#include "BlockchainService.hpp"
#include "ChainDB.hpp"
#include "ConsoleUI.hpp"
#include "DBFile.hpp"
#include "JsonConfig.hpp"
#include "OpenSSLCrypto.hpp"
#include "sha256.hpp"
#include "test_helpers.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

class BlockchainSyncTest : public ::testing::Test
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

    blockchain::Block createBlock(const std::string& previousHash,
                                  const std::string& payload,
                                  const crypto::KeyPair& keyPair)
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

TEST_F(BlockchainSyncTest, SyncEmptyChainToChainWithBlocks)
{
    auto keyPair = crypto->generateKeyPair();

    // Setup peer with blocks
    std::string config1Path = env->createTestConfig(test_helpers::TEST_PORT_PEER1,
                                                    crypto->keyToString(keyPair.privateKey),
                                                    crypto->keyToString(keyPair.publicKey));
    auto config1 = std::make_shared<config::JsonConfig>(config1Path, crypto);

    std::string db1Path = env->createTestDatabase("peer1_sync");
    auto db1 = std::make_shared<db::DBFile>(db1Path);
    auto chainRepo1 = std::make_shared<blockchain::ChainDB>(db1, config1, crypto);
    chainRepo1->init();

    auto consoleUI1 = std::make_shared<ui::ConsoleUI>();
    auto blockchainService1 =
        std::make_shared<blockchain::BlockchainService>(config1, crypto, chainRepo1, consoleUI1);

    // Create chain with 5 blocks
    std::vector<blockchain::Block> blocks;
    blockchain::Block block1 = createBlock("0", "genesis", keyPair);
    chainRepo1->insertBlock(block1);
    blocks.push_back(block1);

    for (int i = 2; i <= 5; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        blockchain::Block block =
            createBlock(blocks.back().hash, "block " + std::to_string(i), keyPair);
        chainRepo1->insertBlock(block);
        blocks.push_back(block);
    }

    EXPECT_TRUE(blockchainService1->validateLocalChain());

    // Setup peer with empty chain
    std::string config2Path = env->createTestConfig(test_helpers::TEST_PORT_PEER2,
                                                    crypto->keyToString(keyPair.privateKey),
                                                    crypto->keyToString(keyPair.publicKey));
    auto config2 = std::make_shared<config::JsonConfig>(config2Path, crypto);

    std::string db2Path = env->createTestDatabase("peer2_sync");
    auto db2 = std::make_shared<db::DBFile>(db2Path);
    auto chainRepo2 = std::make_shared<blockchain::ChainDB>(db2, config2, crypto);
    chainRepo2->init();

    auto consoleUI2 = std::make_shared<ui::ConsoleUI>();
    auto blockchainService2 =
        std::make_shared<blockchain::BlockchainService>(config2, crypto, chainRepo2, consoleUI2);

    // Check missing blocks
    unsigned int missingCount = blockchainService2->countBlocksAfterHash("0");
    EXPECT_EQ(missingCount, 0);  // Empty chain

    missingCount = blockchainService1->countBlocksAfterHash("0");
    EXPECT_EQ(missingCount, 5);

    // Simulate sync: retrieve blocks from peer1
    std::vector<blockchain::Block> retrievedBlocks;
    blockchainService1->getBlocksByIndexRange(0, 5, "0", retrievedBlocks);

    ASSERT_EQ(retrievedBlocks.size(), 5);

    // Add blocks to peer2
    blockchainService2->addNewBlockRange(retrievedBlocks);
    EXPECT_TRUE(blockchainService2->validateNewBlocks());

    // Insert validated blocks
    auto newBlocks = blockchainService2->getNewBlocks();
    for (const auto& block : newBlocks)
    {
        EXPECT_TRUE(chainRepo2->insertBlock(block));
    }

    // Verify sync
    EXPECT_TRUE(blockchainService2->validateLocalChain());

    blockchain::Block tip1, tip2;
    EXPECT_TRUE(chainRepo1->findTip(tip1));
    EXPECT_TRUE(chainRepo2->findTip(tip2));
    EXPECT_EQ(tip1.hash, tip2.hash);

    // Cleanup
    db1->close();
    db2->close();
}

TEST_F(BlockchainSyncTest, SyncPartialChain)
{
    auto keyPair = crypto->generateKeyPair();

    // Setup peer1 with full chain (10 blocks)
    std::string config1Path = env->createTestConfig(test_helpers::TEST_PORT_PEER1,
                                                    crypto->keyToString(keyPair.privateKey),
                                                    crypto->keyToString(keyPair.publicKey));
    auto config1 = std::make_shared<config::JsonConfig>(config1Path, crypto);

    std::string db1Path = env->createTestDatabase("peer1_partial");
    auto db1 = std::make_shared<db::DBFile>(db1Path);
    auto chainRepo1 = std::make_shared<blockchain::ChainDB>(db1, config1, crypto);
    chainRepo1->init();

    auto consoleUI1 = std::make_shared<ui::ConsoleUI>();
    auto blockchainService1 =
        std::make_shared<blockchain::BlockchainService>(config1, crypto, chainRepo1, consoleUI1);

    std::vector<blockchain::Block> allBlocks;
    blockchain::Block block = createBlock("0", "block 1", keyPair);
    chainRepo1->insertBlock(block);
    allBlocks.push_back(block);

    for (int i = 2; i <= 10; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        block = createBlock(allBlocks.back().hash, "block " + std::to_string(i), keyPair);
        chainRepo1->insertBlock(block);
        allBlocks.push_back(block);
    }

    // Setup peer2 with partial chain (first 5 blocks)
    std::string config2Path = env->createTestConfig(test_helpers::TEST_PORT_PEER2,
                                                    crypto->keyToString(keyPair.privateKey),
                                                    crypto->keyToString(keyPair.publicKey));
    auto config2 = std::make_shared<config::JsonConfig>(config2Path, crypto);

    std::string db2Path = env->createTestDatabase("peer2_partial");
    auto db2 = std::make_shared<db::DBFile>(db2Path);
    auto chainRepo2 = std::make_shared<blockchain::ChainDB>(db2, config2, crypto);
    chainRepo2->init();

    auto consoleUI2 = std::make_shared<ui::ConsoleUI>();
    auto blockchainService2 =
        std::make_shared<blockchain::BlockchainService>(config2, crypto, chainRepo2, consoleUI2);

    for (int i = 0; i < 5; ++i)
    {
        chainRepo2->insertBlock(allBlocks[i]);
    }

    blockchain::Block tip2;
    chainRepo2->findTip(tip2);

    // Check missing blocks
    unsigned int missing = blockchainService1->countBlocksAfterHash(tip2.hash);
    EXPECT_EQ(missing, 5);

    // Sync missing blocks
    std::vector<blockchain::Block> missingBlocks;
    blockchainService1->getBlocksByIndexRange(0, 5, tip2.hash, missingBlocks);

    ASSERT_EQ(missingBlocks.size(), 5);

    blockchainService2->addNewBlockRange(missingBlocks);
    EXPECT_TRUE(blockchainService2->validateNewBlocks());

    for (const auto& b : blockchainService2->getNewBlocks())
    {
        EXPECT_TRUE(chainRepo2->insertBlock(b));
    }

    // Verify both chains are now identical
    blockchain::Block tip1;
    chainRepo1->findTip(tip1);
    chainRepo2->findTip(tip2);

    EXPECT_EQ(tip1.hash, tip2.hash);
    EXPECT_TRUE(blockchainService2->validateLocalChain());

    // Cleanup
    db1->close();
    db2->close();
}

TEST_F(BlockchainSyncTest, DetectInvalidBlocksInSync)
{
    auto keyPair = crypto->generateKeyPair();

    std::string configPath = env->createTestConfig(test_helpers::TEST_PORT_BASE,
                                                   crypto->keyToString(keyPair.privateKey),
                                                   crypto->keyToString(keyPair.publicKey));
    auto config = std::make_shared<config::JsonConfig>(configPath, crypto);

    std::string dbPath = env->createTestDatabase("invalid_sync");
    auto db = std::make_shared<db::DBFile>(dbPath);
    auto chainRepo = std::make_shared<blockchain::ChainDB>(db, config, crypto);
    chainRepo->init();

    auto consoleUI = std::make_shared<ui::ConsoleUI>();
    auto blockchainService =
        std::make_shared<blockchain::BlockchainService>(config, crypto, chainRepo, consoleUI);

    // Create valid chain
    blockchain::Block block1 = createBlock("0", "block 1", keyPair);
    chainRepo->insertBlock(block1);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    blockchain::Block block2 = createBlock(block1.hash, "block 2", keyPair);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Create invalid block (wrong previousHash)
    blockchain::Block block3 = createBlock("wrong_hash", "block 3", keyPair);

    // Try to sync blocks with broken chain
    std::vector<blockchain::Block> blocksToSync = { block2, block3 };
    blockchainService->addNewBlockRange(blocksToSync);

    // Validation should fail
    EXPECT_FALSE(blockchainService->validateNewBlocks());

    // Cleanup
    db->close();
}