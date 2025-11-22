#include "ErrorMessage.hpp"

namespace message
{
ErrorMessageResponse::ErrorMessageResponse() : Message(), payload() {}

ErrorMessageResponse::ErrorMessageResponse(const std::string& id,
                                           const peer::UserPeer& from,
                                           const peer::UserPeer& to,
                                           uint64_t timestamp,
                                           const std::string& error)
    : Message(id, MessageType::ERROR_RESPONSE, from, to, timestamp), payload{ error }
{
}

ErrorMessageResponse::ErrorMessageResponse(const json& jData) : Message(jData), payload{}
{
    if (type != MessageType::ERROR_RESPONSE) throw std::runtime_error("Invalid message type");

    payload.error = jData["payload"]["error"].get<std::string>();
}

void ErrorMessageResponse::serialize(json& jData) const
{
    jData = getBasicSerialization();
    jData["payload"]["error"] = payload.error;
}

const ErrorMessageResponsePayload& ErrorMessageResponse::getPayload() const { return payload; }

}  // namespace message