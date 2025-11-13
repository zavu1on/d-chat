#include "DisconnectionMessage.hpp"

namespace message
{
DisconnectionMessage::DisconnectionMessage() : Message() {}

DisconnectionMessage::DisconnectionMessage(const peer::UserPeer& from,
                                           const peer::UserPeer& to,
                                           uint64_t timestamp)
    : Message(MessageType::DISCONNECT, from, to, timestamp)
{
}

DisconnectionMessage::DisconnectionMessage(const json& json) : Message(json)
{
    if (type != MessageType::DISCONNECT) throw std::runtime_error("Invalid message type");
}

void DisconnectionMessage::serialize(json& json) const { json = getBasicSerialization(); }

DisconnectionMessageResponse::DisconnectionMessageResponse() : Message() {}

DisconnectionMessageResponse::DisconnectionMessageResponse(const peer::UserPeer& from,
                                                           const peer::UserPeer& to,
                                                           uint64_t timestamp)
    : Message(MessageType::DISCONNECT_RESPONSE, from, to, timestamp)
{
}

DisconnectionMessageResponse::DisconnectionMessageResponse(const json& json) : Message(json)
{
    if (type != MessageType::DISCONNECT_RESPONSE) throw std::runtime_error("Invalid message type");
}

void DisconnectionMessageResponse::serialize(json& json) const { json = getBasicSerialization(); }
}  // namespace message