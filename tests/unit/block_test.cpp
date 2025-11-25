#include "Block.hpp"

#include <gtest/gtest.h>

#include "OpenSSLCrypto.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

class BlockTest : public ::testing::Test
{
protected:
    std::shared_ptr<crypto::ICrypto> crypto;

    void SetUp() override { crypto = std::make_shared<crypto::OpenSSLCrypto>(); }
};

TEST_F(BlockTest, DefaultConstructorInitializesCorrectly)
{
    blockchain::Block block;

    EXPECT_EQ(block.hash, "0");
    EXPECT_EQ(block.previousHash, "0");
    EXPECT_EQ(block.payloadHash, "0");
    EXPECT_EQ(block.authorPublicKey, "0");
    EXPECT_EQ(block.signature, "0");
    EXPECT_EQ(block.timestamp, 0);
}

TEST_F(BlockTest, ParameterizedConstructorWorks)
{
    std::string hash = "hash123";
    std::string prevHash = "prevHash456";
    std::string payloadHash = "payloadHash789";
    std::string authorKey = "authorKey";
    std::string signature = "signature";
    uint64_t timestamp = utils::getTimestamp();

    blockchain::Block block(hash, prevHash, payloadHash, authorKey, signature, timestamp);

    EXPECT_EQ(block.hash, hash);
    EXPECT_EQ(block.previousHash, prevHash);
    EXPECT_EQ(block.payloadHash, payloadHash);
    EXPECT_EQ(block.authorPublicKey, authorKey);
    EXPECT_EQ(block.signature, signature);
    EXPECT_EQ(block.timestamp, timestamp);
}

TEST_F(BlockTest, ComputeHashProducesConsistentHash)
{
    blockchain::Block block;
    block.previousHash = "prev123";
    block.payloadHash = "payload456";
    block.authorPublicKey = "author789";
    block.timestamp = 1234567890;

    block.computeHash();
    std::string hash1 = block.hash;

    block.hash = "";
    block.computeHash();
    std::string hash2 = block.hash;

    EXPECT_EQ(hash1, hash2);
    EXPECT_NE(hash1, "");
}

TEST_F(BlockTest, DifferentBlocksHaveDifferentHashes)
{
    blockchain::Block block1;
    block1.previousHash = "prev1";
    block1.payloadHash = "payload1";
    block1.authorPublicKey = "author1";
    block1.timestamp = 1000;
    block1.computeHash();

    blockchain::Block block2;
    block2.previousHash = "prev2";
    block2.payloadHash = "payload2";
    block2.authorPublicKey = "author2";
    block2.timestamp = 2000;
    block2.computeHash();

    EXPECT_NE(block1.hash, block2.hash);
}

TEST_F(BlockTest, ToStringForHashIncludesAllFields)
{
    blockchain::Block block;
    block.previousHash = "prev";
    block.payloadHash = "payload";
    block.authorPublicKey = "author";
    block.timestamp = 1234567890;

    std::string canonical = block.toStringForHash();

    EXPECT_NE(canonical.find("prev"), std::string::npos);
    EXPECT_NE(canonical.find("payload"), std::string::npos);
    EXPECT_NE(canonical.find("author"), std::string::npos);
    EXPECT_NE(canonical.find("1234567890"), std::string::npos);
}

TEST_F(BlockTest, JsonSerializationRoundTrip)
{
    blockchain::Block original;
    original.hash = "hash123";
    original.previousHash = "prevHash456";
    original.payloadHash = "payloadHash789";
    original.authorPublicKey = "authorKey";
    original.signature = "signature";
    original.timestamp = 1234567890;

    nlohmann::json jData = original.toJson();
    blockchain::Block recovered(jData);

    EXPECT_EQ(recovered.hash, original.hash);
    EXPECT_EQ(recovered.previousHash, original.previousHash);
    EXPECT_EQ(recovered.payloadHash, original.payloadHash);
    EXPECT_EQ(recovered.authorPublicKey, original.authorPublicKey);
    EXPECT_EQ(recovered.signature, original.signature);
    EXPECT_EQ(recovered.timestamp, original.timestamp);
}

TEST_F(BlockTest, ModifyingFieldChangesHash)
{
    blockchain::Block block;
    block.previousHash = "prev";
    block.payloadHash = "payload";
    block.authorPublicKey = "author";
    block.timestamp = 1000;
    block.computeHash();
    std::string hash1 = block.hash;

    block.timestamp = 2000;
    block.computeHash();
    std::string hash2 = block.hash;

    EXPECT_NE(hash1, hash2);
}