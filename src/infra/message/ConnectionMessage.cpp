#include "ConnectionMessage.hpp"

#include <string>

namespace message
{
ConnectionMessage::ConnectionMessage() : Message(), payload{} {}

ConnectionMessage::ConnectionMessage(const std::string& id,
                                     const peer::UserPeer& from,
                                     const peer::UserPeer& to,
                                     uint64_t timestamp,
                                     const std::string& lastHash)
    : Message(id, MessageType::CONNECT, from, to, timestamp), payload{ lastHash }
{
}

ConnectionMessage::ConnectionMessage(const json& jData) : Message(jData), payload{}
{
    if (type != MessageType::CONNECT) throw std::runtime_error("Invalid message type");
    payload.lastBlockHash = jData["payload"]["lastBlockHash"].get<std::string>();
}

void ConnectionMessage::serialize(json& jData) const
{
    jData = getBasicSerialization();
    jData["payload"]["lastBlockHash"] = payload.lastBlockHash;
}

const ConnectionMessagePayload& ConnectionMessage::getPayload() const { return payload; }

ConnectionMessageResponse::ConnectionMessageResponse() : Message() {}

ConnectionMessageResponse::ConnectionMessageResponse(const std::string& id,
                                                     const peer::UserPeer& from,
                                                     const peer::UserPeer& to,
                                                     uint64_t timestamp,
                                                     unsigned int peersToReceive,
                                                     unsigned int missingBlocksCount)
    : Message(id, MessageType::CONNECT_RESPONSE, from, to, timestamp),
      payload{ peersToReceive, missingBlocksCount }
{
}

ConnectionMessageResponse::ConnectionMessageResponse(const json& jData) : Message(jData), payload{}
{
    if (type != MessageType::CONNECT_RESPONSE) throw std::runtime_error("Invalid message type");

    payload.peersToReceive = jData["payload"]["peersToReceive"].get<unsigned int>();
    payload.missingBlocksCount = jData["payload"]["missingBlocksCount"].get<unsigned int>();
}

void ConnectionMessageResponse::serialize(json& jData) const
{
    jData = getBasicSerialization();
    jData["payload"]["peersToReceive"] = payload.peersToReceive;
    jData["payload"]["missingBlocksCount"] = payload.missingBlocksCount;
}

const ConnectionMessageResponsePayload& ConnectionMessageResponse::getPayload() const
{
    return payload;
}
}  // namespace message