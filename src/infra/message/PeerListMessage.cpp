#include "PeerListMessage.hpp"

#include "timestamp.hpp"
#include "uuid.hpp"

namespace message
{
PeerListMessage::PeerListMessage() : Message(), payload{} {}

PeerListMessage::PeerListMessage(const std::string& id,
                                 const peer::UserPeer& from,
                                 const peer::UserPeer& to,
                                 uint64_t timestamp,
                                 u_int start,
                                 u_int count)
    : Message(id, MessageType::PEER_LIST, from, to, timestamp), payload{ start, count }
{
}

PeerListMessage::PeerListMessage(const json& jData) : Message(jData)
{
    if (type != MessageType::PEER_LIST) throw std::runtime_error("Invalid message type");

    payload.start = jData["payload"]["start"].get<u_int>();
    payload.count = jData["payload"]["count"].get<u_int>();
}

void PeerListMessage::serialize(json& jData) const
{
    jData = getBasicSerialization();

    jData["payload"]["start"] = payload.start;
    jData["payload"]["count"] = payload.count;
}

const PeerListMessagePayload& PeerListMessage::getPayload() const { return payload; }

PeerListMessage PeerListMessage::create(const peer::UserPeer& from,
                                        const peer::UserPeer& to,
                                        u_int start,
                                        u_int count)
{
    return PeerListMessage(utils::uuidv4(), from, to, utils::getTimestamp(), start, count);
}

PeerListMessageResponse::PeerListMessageResponse() : Message() {}

PeerListMessageResponse::PeerListMessageResponse(const std::string& id,
                                                 const peer::UserPeer& from,
                                                 const peer::UserPeer& to,
                                                 uint64_t timestamp,
                                                 const std::vector<peer ::UserPeer> peers)
    : Message(id, MessageType::PEER_LIST_RESPONSE, from, to, timestamp), payload{ peers }
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

PeerListMessageResponse PeerListMessageResponse::create(const peer::UserPeer& from,
                                                        const peer::UserPeer& to,
                                                        const std::vector<peer::UserPeer>& peers)
{
    return PeerListMessageResponse(utils::uuidv4(), to, from, utils::getTimestamp(), peers);
}
}  // namespace message