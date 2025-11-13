#pragma once

#include "Message.hpp"

struct SendMessagePayload
{
    std::string message;
};

class SendMessage : public SecretMessage
{
private:
    Bytes createSessionKey(const std::string& privateKey,
                           const std::string& publicKey,
                           std::shared_ptr<ICrypto> crypto) const;

public:
    SendMessagePayload payload;

    SendMessage();
    SendMessage(const UserPeer& from,
                const UserPeer& to,
                uint64_t timestamp,
                const std::string& message);
    SendMessage(const json& json, const std::string& privateKey, std::shared_ptr<ICrypto> crypto);

    void serialize(json& json,
                   const std::string& privateKey,
                   std::shared_ptr<ICrypto> crypto) const override;
};

class SendResponseMessage : public Message
{
public:
    SendResponseMessage();
    SendResponseMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);
    SendResponseMessage(const json& json);

    void serialize(json& json) const override;
};

using MySendMessage = SendMessage;