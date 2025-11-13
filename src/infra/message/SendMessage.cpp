#include "SendMessage.hpp"

Bytes SendMessage::createSessionKey(const std::string& privateKey,
                                    const std::string& publicKey,
                                    std::shared_ptr<ICrypto> crypto) const
{
    Bytes privateKeyBytes = crypto->stringToKey(privateKey);
    Bytes publicKeyBytes = crypto->stringToKey(publicKey);

    return crypto->createSessionKey(privateKeyBytes, publicKeyBytes);
}

SendMessage::SendMessage() : SecretMessage(), payload() {}

SendMessage::SendMessage(const UserPeer& from,
                         const UserPeer& to,
                         uint64_t timestamp,
                         const std::string& message)
    : SecretMessage(MessageType::SEND_MESSAGE, from, to, timestamp, Bytes()), payload{ message }
{
}

SendMessage::SendMessage(const json& json,
                         const std::string& privateKey,
                         std::shared_ptr<ICrypto> crypto)
    : SecretMessage(json, crypto), payload{}
{
    if (type != MessageType::SEND_MESSAGE) throw std::runtime_error("Invalid message type");

    Bytes sessionKey = createSessionKey(privateKey, from.publicKey, crypto);

    Bytes cipher = crypto->stringToKey(json["payload"]["message"].get<std::string>());
    Bytes plain = crypto->decrypt(cipher, sessionKey);
    Bytes publicKeyBytes = crypto->stringToKey(from.publicKey);
    if (!crypto->verify(plain, signature, publicKeyBytes))
        throw std::runtime_error("Invalid signature");

    payload.message = std::string(plain.begin(), plain.end());
}

void SendMessage::serialize(json& json,
                            const std::string& privateKey,
                            std::shared_ptr<ICrypto> crypto) const
{
    Bytes plain(payload.message.begin(), payload.message.end());
    Bytes sessionKey = createSessionKey(privateKey, to.publicKey, crypto);
    Bytes encryptedMessage = crypto->encrypt(plain, sessionKey);

    Bytes privateKeyBytes = crypto->stringToKey(privateKey);
    Bytes signatureBytes = crypto->sign(plain, privateKeyBytes);

    json = getBasicSerialization();
    json["payload"]["message"] = crypto->keyToString(encryptedMessage);
    json["signature"] = crypto->keyToString(signatureBytes);
}

SendResponseMessage::SendResponseMessage() : Message() {}

SendResponseMessage::SendResponseMessage(const UserPeer& from,
                                         const UserPeer& to,
                                         uint64_t timestamp)
    : Message(MessageType::SEND_MESSAGE_RESPONSE, from, to, timestamp)
{
}

SendResponseMessage::SendResponseMessage(const json& json) : Message(json) {}

void SendResponseMessage::serialize(json& json) const { json = getBasicSerialization(); }
