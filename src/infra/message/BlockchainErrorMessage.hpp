#pragma once
#include "Block.hpp"
#include "Message.hpp"

namespace message
{
struct BlockchainErrorMessageResponsePayload
{
    std::string error;
    blockchain::Block block;
};

class BlockchainErrorMessageResponse : public Message
{
protected:
    BlockchainErrorMessageResponsePayload payload;

public:
    BlockchainErrorMessageResponse();
    BlockchainErrorMessageResponse(const std::string& id,
                                   const peer::UserPeer& from,
                                   uint64_t timestamp,
                                   const std::string& error,
                                   const blockchain::Block& block);
    BlockchainErrorMessageResponse(const json& jData);

    void serialize(json& jData) const override;
    const BlockchainErrorMessageResponsePayload& getPayload() const;

    static BlockchainErrorMessageResponse create(const peer::UserPeer& from,
                                                 const std::string& error,
                                                 const blockchain::Block& block);
};
}  // namespace message