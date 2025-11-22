#pragma once
#include <nlohmann/json.hpp>

#include "Block.hpp"
#include "Message.hpp"

namespace message
{
using json = nlohmann::json;
using u_int = unsigned int;

struct BlockRangeMessagePayload
{
    u_int start;
    u_int count;
    std::string lastHash;
};

class BlockRangeMessage : public Message
{
protected:
    BlockRangeMessagePayload payload;

public:
    BlockRangeMessage();
    BlockRangeMessage(const std::string& id,
                      const peer::UserPeer& from,
                      const peer::UserPeer& to,
                      uint64_t timestamp,
                      u_int start,
                      u_int count,
                      const std::string& lastHash);
    BlockRangeMessage(const json& jData);

    void serialize(json& jData) const override;
    const BlockRangeMessagePayload& getPayload() const;
};

struct BlockRangeMessageResponsePayload
{
    std::vector<blockchain::Block> blocks;
};

class BlockRangeMessageResponse : public Message
{
protected:
    BlockRangeMessageResponsePayload payload;

public:
    BlockRangeMessageResponse();
    BlockRangeMessageResponse(const std::string& id,
                              const peer::UserPeer& from,
                              const peer::UserPeer& to,
                              uint64_t timestamp,
                              const std::vector<blockchain::Block>& blocks);
    BlockRangeMessageResponse(const json& jData);

    void serialize(json& jData) const override;
    const BlockRangeMessageResponsePayload& getPayload() const;
};
}  // namespace message
