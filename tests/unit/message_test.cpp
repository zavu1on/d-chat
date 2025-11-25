#include <gtest/gtest.h>

#include "ConnectionMessage.hpp"
#include "OpenSSLCrypto.hpp"
#include "PeerListMessage.hpp"
#include "TextMessage.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

class MessageTest : public ::testing::Test
{
protected:
    std::shared_ptr<crypto::ICrypto> crypto;
    peer::UserPeer from;
    peer::UserPeer to;

    void SetUp() override
    {
        crypto = std::make_shared<crypto::OpenSSLCrypto>();

        auto keyPair1 = crypto->generateKeyPair();
        auto keyPair2 = crypto->generateKeyPair();

        from = peer::UserPeer("127.0.0.1", 8001, crypto->keyToString(keyPair1.publicKey));
        to = peer::UserPeer("127.0.0.1", 8002, crypto->keyToString(keyPair2.publicKey));
    }
};

TEST_F(MessageTest, ConnectionMessageSerializationWorks)
{
    std::string id = utils::uuidv4();
    uint64_t timestamp = utils::getTimestamp();
    std::string lastHash = "lasthash123";

    message::ConnectionMessage original(id, from, to, timestamp, lastHash);

    nlohmann::json jData;
    original.serialize(jData);

    message::ConnectionMessage recovered(jData);

    EXPECT_EQ(recovered.getId(), original.getId());
    EXPECT_EQ(recovered.getFrom().host, original.getFrom().host);
    EXPECT_EQ(recovered.getFrom().port, original.getFrom().port);
    EXPECT_EQ(recovered.getTo().host, original.getTo().host);
    EXPECT_EQ(recovered.getTo().port, original.getTo().port);
    EXPECT_EQ(recovered.getPayload().lastBlockHash, original.getPayload().lastBlockHash);
}

TEST_F(MessageTest, PeerListMessageSerializationWorks)
{
    std::string id = utils::uuidv4();
    uint64_t timestamp = utils::getTimestamp();
    unsigned int start = 0;
    unsigned int count = 10;

    message::PeerListMessage original(id, from, to, timestamp, start, count);

    nlohmann::json jData;
    original.serialize(jData);

    message::PeerListMessage recovered(jData);

    EXPECT_EQ(recovered.getId(), original.getId());
    EXPECT_EQ(recovered.getPayload().start, original.getPayload().start);
    EXPECT_EQ(recovered.getPayload().count, original.getPayload().count);
}

TEST_F(MessageTest, MessageTypeConversionWorks)
{
    EXPECT_EQ(message::Message::fromMessageTypeToString(message::MessageType::CONNECT), "CONNECT");
    EXPECT_EQ(message::Message::fromMessageTypeToString(message::MessageType::TEXT_MESSAGE),
              "TEXT_MESSAGE");

    EXPECT_EQ(message::Message::fromStringToMessageType("CONNECT"), message::MessageType::CONNECT);
    EXPECT_EQ(message::Message::fromStringToMessageType("TEXT_MESSAGE"),
              message::MessageType::TEXT_MESSAGE);
}

TEST_F(MessageTest, InvalidMessageTypeThrows)
{
    EXPECT_THROW(
        { message::Message::fromStringToMessageType("INVALID_TYPE"); }, std::runtime_error);
}