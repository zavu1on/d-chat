#include "DisconnectionMessage.hpp"

namespace message
{
DisconnectionMessage::DisconnectionMessage() : Message() {}

DisconnectionMessage::DisconnectionMessage(const std::string& id,
                                           const peer::UserPeer& from,
                                           const peer::UserPeer& to,
                                           uint64_t timestamp)
    : Message(id, MessageType::DISCONNECT, from, to, timestamp)
{
}

DisconnectionMessage::DisconnectionMessage(const json& jData) : Message(jData)
{
    if (type != MessageType::DISCONNECT) throw std::runtime_error("Invalid message type");
}

void DisconnectionMessage::serialize(json& jData) const { jData = getBasicSerialization(); }

DisconnectionMessageResponse::DisconnectionMessageResponse() : Message() {}

DisconnectionMessageResponse::DisconnectionMessageResponse(const std::string& id,
                                                           const peer::UserPeer& from,
                                                           const peer::UserPeer& to,
                                                           uint64_t timestamp)
    : Message(id, MessageType::DISCONNECT_RESPONSE, from, to, timestamp)
{
}

DisconnectionMessageResponse::DisconnectionMessageResponse(const json& jData) : Message(jData)
{
    if (type != MessageType::DISCONNECT_RESPONSE) throw std::runtime_error("Invalid message type");
}

void DisconnectionMessageResponse::serialize(json& jData) const { jData = getBasicSerialization(); }
}  // namespace message