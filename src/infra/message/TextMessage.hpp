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
                                   const std::shared_ptr<crypto::ICrypto>& crypto) const;

protected:
    TextMessagePayload payload;

public:
    TextMessage();
    TextMessage(const std::string& id,
                const peer::UserPeer& from,
                const peer::UserPeer& to,
                uint64_t timestamp,
                const std::string& message,
                const std::string& blockHash);
    TextMessage(const json& jData,
                const std::string& privateKey,
                const std::shared_ptr<crypto::ICrypto>& crypto,
                bool invertFromTo = false,
                bool createObjectIfError = false);

    void serialize(json& jData,
                   const std::string& privateKey,
                   const std::shared_ptr<crypto::ICrypto>& crypto) const override;
    const TextMessagePayload& getPayload() const;
    void setBlockHash(const std::string& blockHash);
};

class TextMessageResponse : public Message
{
public:
    TextMessageResponse();
    TextMessageResponse(const std::string& id,
                        const peer::UserPeer& from,
                        const peer::UserPeer& to,
                        uint64_t timestamp);
    TextMessageResponse(const json& jData);

    void serialize(json& jData) const override;
};
}  // namespace message
