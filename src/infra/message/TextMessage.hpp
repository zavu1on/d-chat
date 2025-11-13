#pragma once

#include <nlohmann/json.hpp>

#include "Message.hpp"

namespace message
{
using json = nlohmann::json;

struct TextMessagePayload
{
    std::string message;
};

class TextMessage : public SecretMessage
{
private:
    crypto::Bytes createSessionKey(const std::string& privateKey,
                                   const std::string& publicKey,
                                   std::shared_ptr<crypto::ICrypto> crypto) const;

public:
    TextMessagePayload payload;

    TextMessage();
    TextMessage(const peer::UserPeer& from,
                const peer::UserPeer& to,
                uint64_t timestamp,
                const std::string& message);
    TextMessage(const json& json,
                const std::string& privateKey,
                std::shared_ptr<crypto::ICrypto> crypto);

    void serialize(json& json,
                   const std::string& privateKey,
                   std::shared_ptr<crypto::ICrypto> crypto) const override;
};

class TextMessageResponse : public Message
{
public:
    TextMessageResponse();
    TextMessageResponse(const peer::UserPeer& from, const peer::UserPeer& to, uint64_t timestamp);
    TextMessageResponse(const json& json);

    void serialize(json& json) const override;
};
}  // namespace message
