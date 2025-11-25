#include <gtest/gtest.h>

#include <chrono>
#include <set>
#include <thread>

#include "hex.hpp"
#include "sha256.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"


TEST(UtilsTest, UUIDv4GeneratesUniqueIDs)
{
    std::set<std::string> uuids;
    const int count = 1000;

    for (int i = 0; i < count; ++i)
    {
        std::string uuid = utils::uuidv4();
        EXPECT_EQ(uuid.length(), 36);  // UUID format: 8-4-4-4-12
        uuids.insert(uuid);
    }

    // All UUIDs should be unique
    EXPECT_EQ(uuids.size(), count);
}

TEST(UtilsTest, UUIDv4HasCorrectFormat)
{
    std::string uuid = utils::uuidv4();

    // Check structure: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    EXPECT_EQ(uuid[8], '-');
    EXPECT_EQ(uuid[13], '-');
    EXPECT_EQ(uuid[18], '-');
    EXPECT_EQ(uuid[23], '-');
    EXPECT_EQ(uuid[14], '4');  // Version 4
}

TEST(UtilsTest, TimestampIncreases)
{
    uint64_t ts1 = utils::getTimestamp();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    uint64_t ts2 = utils::getTimestamp();

    EXPECT_GT(ts2, ts1);
}

TEST(UtilsTest, TimestampIsInMilliseconds)
{
    uint64_t ts = utils::getTimestamp();

    // Timestamp should be reasonably close to current time
    // (e.g., after year 2020 in milliseconds)
    EXPECT_GT(ts, 1577836800000ULL);  // Jan 1, 2020 in ms
}

TEST(UtilsTest, SHA256ProducesConsistentHash)
{
    std::string data = "Test data for hashing";

    std::string hash1 = utils::sha256(data);
    std::string hash2 = utils::sha256(data);

    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash1.length(), 64);  // SHA256 produces 64 hex characters
}

TEST(UtilsTest, SHA256DifferentDataProducesDifferentHash)
{
    std::string hash1 = utils::sha256("data1");
    std::string hash2 = utils::sha256("data2");

    EXPECT_NE(hash1, hash2);
}

TEST(UtilsTest, SHA256EmptyStringWorks)
{
    std::string hash = utils::sha256("");
    EXPECT_EQ(hash.length(), 64);
    EXPECT_FALSE(hash.empty());
}

TEST(UtilsTest, HexConversionRoundTrip)
{
    std::vector<uint8_t> original = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };

    std::string hex = utils::toHex(original);
    EXPECT_EQ(hex, "0123456789abcdef");

    std::vector<uint8_t> recovered = utils::fromHex(hex);
    EXPECT_EQ(recovered, original);
}

TEST(UtilsTest, HexConversionEmptyVector)
{
    std::vector<uint8_t> empty;
    std::string hex = utils::toHex(empty);
    EXPECT_TRUE(hex.empty());

    std::vector<uint8_t> recovered = utils::fromHex(hex);
    EXPECT_TRUE(recovered.empty());
}

TEST(UtilsTest, HexConversionUpperAndLowerCase)
{
    std::vector<uint8_t> data = { 0xAB, 0xCD };

    std::string hexLower = utils::toHex(data);
    std::vector<uint8_t> recovered1 = utils::fromHex(hexLower);
    EXPECT_EQ(recovered1, data);

    std::vector<uint8_t> recovered2 = utils::fromHex("ABCD");
    EXPECT_EQ(recovered2, data);
}