#include "ConnectMessage.hpp"

ConnectMessage::ConnectMessage() : Message() {}

ConnectMessage::ConnectMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp)
    : Message(MessageType::CONNECT, from, to, timestamp)
{
}

ConnectMessage::ConnectMessage(const json& json) : Message(json)
{
    if (type != MessageType::CONNECT) throw std::runtime_error("Invalid message type");
}

void ConnectMessage::serialize(json& json) const { json = getBasicSerialization(); }

ConnectResponseMessage::ConnectResponseMessage() : Message() {}

ConnectResponseMessage::ConnectResponseMessage(const UserPeer& from,
                                               const UserPeer& to,
                                               uint64_t timestamp)
    : Message(MessageType::CONNECT_RESPONSE, from, to, timestamp)
{
}

ConnectResponseMessage::ConnectResponseMessage(const json& json) : Message(json)
{
    if (type != MessageType::CONNECT_RESPONSE) throw std::runtime_error("Invalid message type");
}

void ConnectResponseMessage::serialize(json& json) const { json = getBasicSerialization(); }
