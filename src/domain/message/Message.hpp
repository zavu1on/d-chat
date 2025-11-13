#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "ICrypto.hpp"
#include "UserPeer.hpp"

namespace message
{
using json = nlohmann::json;

enum class MessageType
{
    NONE,
    CONNECT,
    CONNECT_RESPONSE,
    PEER_LIST,
    PEER_LIST_RESPONSE,
    TEXT_MESSAGE,
    TEXT_MESSAGE_RESPONSE,
    DISCONNECT,
    DISCONNECT_RESPONSE,
};

class Message
{
protected:
    virtual json getBasicSerialization() const;

public:
    MessageType type;
    peer::UserPeer from;
    peer::UserPeer to;
    uint64_t timestamp;

    Message();
    Message(MessageType type,
            const peer::UserPeer& from,
            const peer::UserPeer& to,
            uint64_t timestamp);
    Message(const json& jData);
    virtual ~Message() = default;
    virtual void serialize(json& jData) const = 0;

    static std::string fromMessageTypeToString(MessageType type);
    static MessageType fromStringToMessageType(const std::string& type);
};

class SecretMessage : public Message
{
private:
    void serialize(json& jData) const final;

protected:
    crypto::Bytes signature;

public:
    SecretMessage();
    SecretMessage(MessageType type,
                  const peer::UserPeer& from,
                  const peer::UserPeer& to,
                  uint64_t timestamp,
                  const crypto::Bytes& signature);
    SecretMessage(const json& jData, std::shared_ptr<crypto::ICrypto> crypto);

    virtual void serialize(json& jData,
                           const std::string& privateKey,
                           std::shared_ptr<crypto::ICrypto> crypto) const = 0;
};
}  // namespace message