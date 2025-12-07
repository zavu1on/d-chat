#include "TextMessage.hpp"

namespace message
{
crypto::Bytes TextMessage::createSessionKey(const std::string& privateKey,
                                            const std::string& publicKey,
                                            const std::shared_ptr<crypto::ICrypto>& crypto) const
{
    crypto::Bytes privateKeyBytes = crypto->stringToKey(privateKey);
    crypto::Bytes publicKeyBytes = crypto->stringToKey(publicKey);

    return crypto->createSessionKey(privateKeyBytes, publicKeyBytes);
}

TextMessage::TextMessage() : SecretMessage(), payload() {}

TextMessage::TextMessage(const std::string& id,
                         const peer::UserPeer& from,
                         const peer::UserPeer& to,
                         uint64_t timestamp,
                         const std::string& message,
                         const std::string& blockHash)
    : SecretMessage(id, MessageType::TEXT_MESSAGE, from, to, timestamp, crypto::Bytes(), blockHash),
      payload{ message }
{
}

TextMessage::TextMessage(const json& jData,
                         const std::string& privateKey,
                         const std::shared_ptr<crypto::ICrypto>& crypto,
                         bool invertFromTo,
                         bool createObjectIfError)
    : SecretMessage(jData, crypto), payload{}
{
    try
    {
        if (type != MessageType::TEXT_MESSAGE) throw std::runtime_error("Invalid message type");

        std::string publicKey = invertFromTo ? to.publicKey : from.publicKey;
        crypto::Bytes sessionKey = createSessionKey(privateKey, publicKey, crypto);

        crypto::Bytes cipher = crypto->stringToKey(jData["payload"]["message"].get<std::string>());
        crypto::Bytes plain = crypto->decrypt(cipher, sessionKey);
        crypto::Bytes publicKeyBytes = crypto->stringToKey(from.publicKey);

        if (!crypto->verify(plain, signature, publicKeyBytes))
            throw std::runtime_error("Invalid signature");

        payload.message = std::string(plain.begin(), plain.end());
    }
    catch (const std::exception& error)
    {
        if (!createObjectIfError) throw error;
        payload.message = "CRASHED MESSAGE";
    }
}

void TextMessage::serialize(json& jData,
                            const std::string& privateKey,
                            const std::shared_ptr<crypto::ICrypto>& crypto) const
{
    crypto::Bytes plain(payload.message.begin(), payload.message.end());
    crypto::Bytes sessionKey = createSessionKey(privateKey, to.publicKey, crypto);
    crypto::Bytes encryptedMessage = crypto->encrypt(plain, sessionKey);

    crypto::Bytes privateKeyBytes = crypto->stringToKey(privateKey);
    crypto::Bytes signatureBytes = crypto->sign(plain, privateKeyBytes);

    jData = getBasicSerialization();
    jData["payload"]["message"] = crypto->keyToString(encryptedMessage);
    jData["signature"] = crypto->keyToString(signatureBytes);
}

const TextMessagePayload& TextMessage::getPayload() const { return payload; }

void TextMessage::setBlockHash(const std::string& blockHash) { this->blockHash = blockHash; }

TextMessageResponse::TextMessageResponse() : Message() {}

TextMessageResponse::TextMessageResponse(const std::string& id,
                                         const peer::UserPeer& from,
                                         const peer::UserPeer& to,
                                         uint64_t timestamp)
    : Message(id, MessageType::TEXT_MESSAGE_RESPONSE, from, to, timestamp)
{
}

TextMessageResponse::TextMessageResponse(const json& jData) : Message(jData) {}

void TextMessageResponse::serialize(json& jData) const { jData = getBasicSerialization(); }
}  // namespace message