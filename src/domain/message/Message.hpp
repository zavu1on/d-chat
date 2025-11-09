#pragma once

#include <nlohmann/json.hpp>
#include <string>

#include "UserPeer.hpp"

using json = nlohmann::json;

enum class MessageType
{
    NONE,
    CONNECT,
    CONNECT_RESPONSE,
    PEER_LIST,
    PEER_LIST_RESPONSE,
    // ...
    DISCONNECT,
    DISCONNECT_RESPONSE,
};

class Message
{
protected:
    json getBasicSerialization() const;

public:
    MessageType type;
    UserPeer from;
    UserPeer to;
    uint64_t timestamp;

    Message();
    Message(MessageType type, const UserPeer& from, const UserPeer& to, uint64_t timestamp);
    virtual ~Message() = default;
    virtual void serialize(json& json) const = 0;

    static std::string fromMessageTypeToString(MessageType type);
    static MessageType fromStringToMessageType(const std::string& type);
};