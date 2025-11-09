#include "DisconnectMessage.hpp"

void DisconnectMessage::deserializeBasic(const json& json, DisconnectMessage& message)
{
    message.from = UserPeer(json["from"]);
    message.to = UserPeer(json["to"]);
    message.timestamp = json["timestamp"].get<uint64_t>();
}

DisconnectMessage::DisconnectMessage() : Message() {}

DisconnectMessage::DisconnectMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp)
    : Message(MessageType::DISCONNECT, from, to, timestamp)
{
}

void DisconnectMessage::serialize(json& json) const { json = getBasicSerialization(); }

DisconnectMessage DisconnectMessage::fromJson(const json& json)
{
    if (json["type"] != Message::fromMessageTypeToString(MessageType::DISCONNECT))
        throw std::runtime_error("Invalid message type");

    DisconnectMessage message;
    deserializeBasic(json, message);

    return message;
}

DisconnectResponseMessage::DisconnectResponseMessage() : DisconnectMessage() {}

DisconnectResponseMessage::DisconnectResponseMessage(const UserPeer& from,
                                                     const UserPeer& to,
                                                     uint64_t timestamp)
    : DisconnectMessage(from, to, timestamp)
{
    type = MessageType::DISCONNECT_RESPONSE;
}

DisconnectResponseMessage DisconnectResponseMessage::fromJson(const json& json)
{
    if (json["type"] != Message::fromMessageTypeToString(MessageType::DISCONNECT_RESPONSE))
        throw std::runtime_error("Invalid message type");

    DisconnectResponseMessage message;
    deserializeBasic(json, message);

    return message;
}
