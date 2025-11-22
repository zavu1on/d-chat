#include "BlockchainErrorMessage.hpp"

namespace message
{
BlockchainErrorMessageResponse::BlockchainErrorMessageResponse() : Message(), payload() {}

BlockchainErrorMessageResponse::BlockchainErrorMessageResponse(const std::string& id,
                                                               const peer::UserPeer& from,
                                                               uint64_t timestamp,
                                                               const std::string& error,
                                                               const blockchain::Block& block)
    : Message(id, MessageType::BLOCKCHAIN_ERROR_RESPONSE, from, {}, timestamp),
      payload{ error, block }
{
}

BlockchainErrorMessageResponse::BlockchainErrorMessageResponse(const json& jData)
    : Message(jData), payload{}
{
    if (type != MessageType::BLOCKCHAIN_ERROR_RESPONSE)
        throw std::runtime_error("Invalid message type");

    payload.error = jData["payload"]["error"].get<std::string>();
    payload.block = blockchain::Block(jData["payload"]["block"]);
}

void BlockchainErrorMessageResponse::serialize(json& jData) const
{
    jData = getBasicSerialization();
    jData["payload"]["error"] = payload.error;
    jData["payload"]["block"] = payload.block.toJson();
}

const BlockchainErrorMessageResponsePayload& BlockchainErrorMessageResponse::getPayload() const
{
    return payload;
}

}  // namespace message