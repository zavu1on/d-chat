#include "ConnectionMessage.hpp"

#include <string>

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
                                                     uint64_t timestamp,
                                                     unsigned int peersToReceive)
    : Message(MessageType::CONNECT_RESPONSE, from, to, timestamp), payload{ peersToReceive }
{
}

ConnectionMessageResponse::ConnectionMessageResponse(const json& jData) : Message(jData), payload{}
{
    if (type != MessageType::CONNECT_RESPONSE) throw std::runtime_error("Invalid message type");

    payload.peersToReceive = jData["payload"]["peersToReceive"].get<unsigned int>();
}

void ConnectionMessageResponse::serialize(json& jData) const
{
    jData = getBasicSerialization();
    jData["payload"]["peersToReceive"] = payload.peersToReceive;
}

const ConnectionMessageResponsePayload& ConnectionMessageResponse::getPayload() const
{
    return payload;
}
}  // namespace message