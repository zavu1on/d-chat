#pragma once
#include "Block.hpp"
#include "Message.hpp"

namespace message
{
struct BlockchainErrorMessageResponsePayload
{
    std::string error;
    std::string blockHash;
    std::string messageId;
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
                                   const std::string& blockHash,
                                   const std::string& messageId);
    BlockchainErrorMessageResponse(const json& jData);

    void serialize(json& jData) const override;
    const BlockchainErrorMessageResponsePayload& getPayload() const;

    static BlockchainErrorMessageResponse create(const peer::UserPeer& from,
                                                 const std::string& error,
                                                 const std::string& blockHash,
                                                 const std::string& messageId);
};
}  // namespace message