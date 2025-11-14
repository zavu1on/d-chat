#include "PeerListMessage.hpp"

namespace message
{
PeerListMessage::PeerListMessage() : Message(), payload{} {}

PeerListMessage::PeerListMessage(const peer::UserPeer& from,
                                 const peer::UserPeer& to,
                                 uint64_t timestamp,
                                 u_int start,
                                 u_int end)
    : Message(MessageType::PEER_LIST, from, to, timestamp), payload{ start, end }
{
}

PeerListMessage::PeerListMessage(const json& jData) : Message(jData)
{
    if (type != MessageType::PEER_LIST) throw std::runtime_error("Invalid message type");

    payload.start = jData["payload"]["start"].get<u_int>();
    payload.end = jData["payload"]["end"].get<u_int>();
}

void PeerListMessage::serialize(json& jData) const
{
    jData = getBasicSerialization();

    jData["payload"]["start"] = payload.start;
    jData["payload"]["end"] = payload.end;
}

const PeerListMessagePayload& PeerListMessage::getPayload() const { return payload; }

PeerListMessageResponse::PeerListMessageResponse() : Message() {}

PeerListMessageResponse::PeerListMessageResponse(const peer::UserPeer& from,
                                                 const peer::UserPeer& to,
                                                 uint64_t timestamp,
                                                 const std::vector<peer ::UserPeer> peers)
    : Message(MessageType::PEER_LIST_RESPONSE, from, to, timestamp), payload{ peers }
{
}

PeerListMessageResponse::PeerListMessageResponse(const json& jData) : Message(jData), payload{}
{
    if (type != MessageType::PEER_LIST_RESPONSE) throw std::runtime_error("Invalid message type");

    payload.peers.clear();

    json peersJson = jData["payload"]["peers"];
    if (!peersJson.is_array()) throw std::runtime_error("Peers field must be an array");

    for (const auto& jPeer : peersJson) payload.peers.emplace_back(jPeer);
}

void PeerListMessageResponse::serialize(json& jData) const
{
    jData = getBasicSerialization();

    jData["payload"]["peers"] = nlohmann::json::array();
    for (const auto& peer : payload.peers)
    {
        jData["payload"]["peers"].push_back(peer.toJson());
    }
}
const PeerListMessageResponsePayload& PeerListMessageResponse::getPayload() const
{
    return payload;
}
}  // namespace message