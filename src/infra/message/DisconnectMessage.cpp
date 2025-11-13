#include "DisconnectMessage.hpp"

DisconnectMessage::DisconnectMessage() : Message() {}

DisconnectMessage::DisconnectMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp)
    : Message(MessageType::DISCONNECT, from, to, timestamp)
{
}

DisconnectMessage::DisconnectMessage(const json& json) : Message(json)
{
    if (type != MessageType::DISCONNECT) throw std::runtime_error("Invalid message type");
}

void DisconnectMessage::serialize(json& json) const { json = getBasicSerialization(); }

DisconnectResponseMessage::DisconnectResponseMessage() : Message() {}

DisconnectResponseMessage::DisconnectResponseMessage(const UserPeer& from,
                                                     const UserPeer& to,
                                                     uint64_t timestamp)
    : Message(MessageType::DISCONNECT_RESPONSE, from, to, timestamp)
{
}

DisconnectResponseMessage::DisconnectResponseMessage(const json& json) : Message(json)
{
    if (type != MessageType::DISCONNECT_RESPONSE) throw std::runtime_error("Invalid message type");
}

void DisconnectResponseMessage::serialize(json& json) const { json = getBasicSerialization(); }
