#include "BlockRangeMessage.hpp"

namespace message
{

BlockRangeMessage::BlockRangeMessage() : Message(), payload{ 0, 0, "0" } {}

BlockRangeMessage::BlockRangeMessage(const std::string& id,
                                     const peer::UserPeer& from,
                                     const peer::UserPeer& to,
                                     uint64_t timestamp,
                                     u_int start,
                                     u_int count,
                                     const std::string& lastHash)
    : Message(id, MessageType::BLOCK_RANGE_REQUEST, from, to, timestamp),
      payload{ start, count, lastHash }
{
}

BlockRangeMessage::BlockRangeMessage(const json& jData) : Message(jData)
{
    if (type != MessageType::BLOCK_RANGE_REQUEST) throw std::runtime_error("Invalid message type");
    payload.start = jData["payload"]["start"].get<uint64_t>();
    payload.count = jData["payload"]["count"].get<unsigned int>();
    payload.lastHash = jData["payload"]["lastHash"].get<std::string>();
}

void BlockRangeMessage::serialize(json& jData) const
{
    jData = getBasicSerialization();
    jData["payload"]["start"] = payload.start;
    jData["payload"]["count"] = payload.count;
    jData["payload"]["lastHash"] = payload.lastHash;
}

const BlockRangeMessagePayload& BlockRangeMessage::getPayload() const { return payload; }

BlockRangeMessageResponse::BlockRangeMessageResponse() : Message(), payload() {}

BlockRangeMessageResponse::BlockRangeMessageResponse(const std::string& id,
                                                     const peer::UserPeer& from,
                                                     const peer::UserPeer& to,
                                                     uint64_t timestamp,
                                                     const std::vector<blockchain::Block>& blocks)
    : Message(id, MessageType::BLOCK_RANGE_RESPONSE, from, to, timestamp), payload{ blocks }
{
}

BlockRangeMessageResponse::BlockRangeMessageResponse(const json& jData) : Message(jData), payload()
{
    if (type != MessageType::BLOCK_RANGE_RESPONSE) throw std::runtime_error("Invalid message type");

    payload.blocks.clear();

    json blocksJson = jData["payload"]["blocks"];
    if (!blocksJson.is_array()) throw std::runtime_error("Peers field must be an array");

    for (const auto& jBlock : blocksJson) payload.blocks.emplace_back(jBlock);
}

void BlockRangeMessageResponse::serialize(json& jData) const
{
    jData = getBasicSerialization();

    jData["payload"]["blocks"] = nlohmann::json::array();
    for (const auto& block : payload.blocks)
    {
        jData["payload"]["blocks"].push_back(block.toJson());
    }
}

const BlockRangeMessageResponsePayload& BlockRangeMessageResponse::getPayload() const
{
    return payload;
}
}  // namespace message
