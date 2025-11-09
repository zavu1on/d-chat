#include "ConnectMessage.hpp"

void ConnectMessage::deserializeBasic(const json& json, ConnectMessage& message)
{
    message.from = UserPeer(json["from"]);
    message.to = UserPeer(json["to"]);
    message.timestamp = json["timestamp"].get<uint64_t>();
}

ConnectMessage::ConnectMessage() : Message() {}

ConnectMessage::ConnectMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp)
    : Message(MessageType::CONNECT, from, to, timestamp)
{
}

void ConnectMessage::serialize(json& json) const { json = getBasicSerialization(); }

ConnectMessage ConnectMessage::fromJson(const json& json)
{
    if (json["type"] != Message::fromMessageTypeToString(MessageType::CONNECT))
        throw std::runtime_error("Invalid message type");

    ConnectMessage message;
    deserializeBasic(json, message);

    return message;
}

ConnectResponseMessage::ConnectResponseMessage() : ConnectMessage()
{
    type = MessageType::CONNECT_RESPONSE;
}

ConnectResponseMessage::ConnectResponseMessage(const UserPeer& from,
                                               const UserPeer& to,
                                               uint64_t timestamp)
    : ConnectMessage(from, to, timestamp)
{
    type = MessageType::CONNECT_RESPONSE;
}

void ConnectResponseMessage::serialize(json& json) const { json = getBasicSerialization(); }

ConnectResponseMessage ConnectResponseMessage::fromJson(const json& json)
{
    if (json["type"] != Message::fromMessageTypeToString(MessageType::CONNECT_RESPONSE))
        throw std::runtime_error("Invalid message type");

    ConnectResponseMessage message;
    deserializeBasic(json, message);

    return message;
}
