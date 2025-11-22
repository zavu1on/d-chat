#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "ICrypto.hpp"
#include "UserPeer.hpp"

// todo create message factory

namespace message
{
using json = nlohmann::json;

enum class MessageType
{
    NONE,
    ERROR_RESPONSE,
    BLOCKCHAIN_ERROR_RESPONSE,
    CONNECT,
    CONNECT_RESPONSE,
    PEER_LIST,
    PEER_LIST_RESPONSE,
    BLOCK_RANGE_REQUEST,
    BLOCK_RANGE_RESPONSE,
    TEXT_MESSAGE,
    TEXT_MESSAGE_RESPONSE,
    DISCONNECT,
    DISCONNECT_RESPONSE,
};

class Message
{
protected:
    virtual json getBasicSerialization() const;

    std::string id;
    MessageType type;
    peer::UserPeer from;
    peer::UserPeer to;
    uint64_t timestamp;

public:
    Message();
    Message(const std::string& id,
            MessageType type,
            const peer::UserPeer& from,
            const peer::UserPeer& to,
            uint64_t timestamp);
    Message(const json& jData);
    virtual ~Message() = default;
    virtual void serialize(json& jData) const = 0;

    const std::string& getId() const;
    MessageType getType() const;
    const peer::UserPeer& getFrom() const;
    const peer::UserPeer& getTo() const;
    uint64_t getTimestamp() const;

    static std::string fromMessageTypeToString(MessageType type);
    static MessageType fromStringToMessageType(const std::string& type);
};

class SecretMessage : public Message
{
private:
    void serialize(json& jData) const final;

protected:
    crypto::Bytes signature;
    std::string blockHash;
    json getBasicSerialization() const override;

public:
    SecretMessage();
    SecretMessage(const std::string& id,
                  MessageType type,
                  const peer::UserPeer& from,
                  const peer::UserPeer& to,
                  uint64_t timestamp,
                  const crypto::Bytes& signature,
                  const std::string& blockHash);
    SecretMessage(const json& jData, const std::shared_ptr<crypto::ICrypto>& crypto);
    const crypto::Bytes& getSignature() const;
    const std::string& getBlockHash() const;
    virtual void serialize(json& jData,
                           const std::string& privateKey,
                           const std::shared_ptr<crypto::ICrypto>& crypto) const = 0;
};
}  // namespace message