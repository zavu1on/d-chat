#include "DisconnectionMessage.hpp"

#include "timestamp.hpp"
#include "uuid.hpp"

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

DisconnectionMessage DisconnectionMessage::create(const peer::UserPeer& from,
                                                  const peer::UserPeer& to)
{
    return DisconnectionMessage(utils::uuidv4(), from, to, utils::getTimestamp());
}

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

DisconnectionMessageResponse DisconnectionMessageResponse::create(const peer::UserPeer& from,
                                                                  const peer::UserPeer& to)
{
    return DisconnectionMessageResponse(utils::uuidv4(), from, to, utils::getTimestamp());
}
}  // namespace message