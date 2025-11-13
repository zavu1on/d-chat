#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "ICrypto.hpp"
#include "UserPeer.hpp"

using json = nlohmann::json;

enum class MessageType
{
    NONE,
    CONNECT,
    CONNECT_RESPONSE,
    PEER_LIST,
    PEER_LIST_RESPONSE,
    SEND_MESSAGE,
    SEND_MESSAGE_RESPONSE,
    DISCONNECT,
    DISCONNECT_RESPONSE,
};

class Message
{
protected:
    virtual json getBasicSerialization() const;

public:
    MessageType type;
    UserPeer from;
    UserPeer to;
    uint64_t timestamp;

    Message();
    Message(MessageType type, const UserPeer& from, const UserPeer& to, uint64_t timestamp);
    Message(const json& json);
    virtual ~Message() = default;
    virtual void serialize(json& json) const = 0;

    static std::string fromMessageTypeToString(MessageType type);
    static MessageType fromStringToMessageType(const std::string& type);
};

class SecretMessage : public Message
{
private:
    void serialize(json& json) const final;

protected:
    Bytes signature;

public:
    SecretMessage();
    SecretMessage(MessageType type,
                  const UserPeer& from,
                  const UserPeer& to,
                  uint64_t timestamp,
                  const Bytes& signature);
    SecretMessage(const json& json, std::shared_ptr<ICrypto> crypto);

    virtual void serialize(json& json,
                           const std::string& privateKey,
                           std::shared_ptr<ICrypto> crypto) const = 0;
};