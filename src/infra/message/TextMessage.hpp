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

protected:
    TextMessagePayload payload;

public:
    TextMessage();
    TextMessage(const peer::UserPeer& from,
                const peer::UserPeer& to,
                uint64_t timestamp,
                const std::string& message);
    TextMessage(const json& jData,
                const std::string& privateKey,
                std::shared_ptr<crypto::ICrypto> crypto);

    void serialize(json& jData,
                   const std::string& privateKey,
                   std::shared_ptr<crypto::ICrypto> crypto) const override;
    const TextMessagePayload& getPayload() const;
};

class TextMessageResponse : public Message
{
public:
    TextMessageResponse();
    TextMessageResponse(const peer::UserPeer& from, const peer::UserPeer& to, uint64_t timestamp);
    TextMessageResponse(const json& jData);

    void serialize(json& jData) const override;
};
}  // namespace message
