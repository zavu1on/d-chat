#include "BlockchainErrorMessage.hpp"

#include <timestamp.hpp>
#include <uuid.hpp>

namespace message
{
BlockchainErrorMessageResponse::BlockchainErrorMessageResponse() : Message(), payload() {}

BlockchainErrorMessageResponse::BlockchainErrorMessageResponse(const std::string& id,
                                                               const peer::UserPeer& from,
                                                               uint64_t timestamp,
                                                               const std::string& error,
                                                               const std::string& blockHash,
                                                               const std::string& messageId)
    : Message(id, MessageType::BLOCKCHAIN_ERROR_RESPONSE, from, {}, timestamp),
      payload{ error, blockHash, messageId }
{
}

BlockchainErrorMessageResponse::BlockchainErrorMessageResponse(const json& jData)
    : Message(jData), payload{}
{
    if (type != MessageType::BLOCKCHAIN_ERROR_RESPONSE)
        throw std::runtime_error("Invalid message type");

    payload.error = jData["payload"]["error"].get<std::string>();
    payload.blockHash = jData["payload"]["blockHash"].get<std::string>();
    payload.messageId = jData["payload"]["messageId"].get<std::string>();
}

void BlockchainErrorMessageResponse::serialize(json& jData) const
{
    jData = getBasicSerialization();
    jData["payload"]["error"] = payload.error;
    jData["payload"]["blockHash"] = payload.blockHash;
    jData["payload"]["messageId"] = payload.messageId;
}

const BlockchainErrorMessageResponsePayload& BlockchainErrorMessageResponse::getPayload() const
{
    return payload;
}

BlockchainErrorMessageResponse BlockchainErrorMessageResponse::create(const peer::UserPeer& from,
                                                                      const std::string& error,
                                                                      const std::string& blockHash,
                                                                      const std::string& messageId)
{
    return BlockchainErrorMessageResponse(
        utils::uuidv4(), from, utils::getTimestamp(), error, blockHash, messageId);
}

}  // namespace message