#include "ConnectionMessage.hpp"

namespace message
{
ConnectionMessage::ConnectionMessage() : Message() {}

ConnectionMessage::ConnectionMessage(const peer::UserPeer& from,
                                     const peer::UserPeer& to,
                                     uint64_t timestamp)
    : Message(MessageType::CONNECT, from, to, timestamp)
{
}

ConnectionMessage::ConnectionMessage(const json& jData) : Message(jData)
{
    if (type != MessageType::CONNECT) throw std::runtime_error("Invalid message type");
}

void ConnectionMessage::serialize(json& jData) const { jData = getBasicSerialization(); }

ConnectionMessageResponse::ConnectionMessageResponse() : Message() {}

ConnectionMessageResponse::ConnectionMessageResponse(const peer::UserPeer& from,
                                                     const peer::UserPeer& to,
                                                     uint64_t timestamp)
    : Message(MessageType::CONNECT_RESPONSE, from, to, timestamp)
{
}

ConnectionMessageResponse::ConnectionMessageResponse(const json& jData) : Message(jData)
{
    if (type != MessageType::CONNECT_RESPONSE) throw std::runtime_error("Invalid message type");
}

void ConnectionMessageResponse::serialize(json& jData) const { jData = getBasicSerialization(); }
}  // namespace message