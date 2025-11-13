#include "Message.hpp"

#include <stdexcept>

namespace message
{
json Message::getBasicSerialization() const
{
    json jData;
    jData["type"] = Message::fromMessageTypeToString(type);
    jData["from"] = from.toJson();
    jData["to"] = to.toJson();
    jData["timestamp"] = timestamp;
    return jData;
}

Message::Message() : type(MessageType::NONE), from(), to(), timestamp(0) {}

Message::Message(MessageType type,
                 const peer::UserPeer& from,
                 const peer::UserPeer& to,
                 uint64_t timestamp)
    : type(type), from(from), to(to), timestamp(timestamp)
{
}

Message::Message(const json& jData)
{
    from = peer::UserPeer(jData["from"]);
    to = peer::UserPeer(jData["to"]);
    timestamp = jData["timestamp"].get<uint64_t>();
    type = Message::fromStringToMessageType(jData["type"].get<std::string>());
}

std::string Message::fromMessageTypeToString(MessageType type)
{
    switch (type)
    {
        case MessageType::NONE:
            return "NONE";
        case MessageType::CONNECT:
            return "CONNECT";
        case MessageType::CONNECT_RESPONSE:
            return "CONNECT_RESPONSE";
        case MessageType::PEER_LIST:
            return "PEER_LIST";
        case MessageType::PEER_LIST_RESPONSE:
            return "PEER_LIST_RESPONSE";
        case MessageType::TEXT_MESSAGE:
            return "TEXT_MESSAGE";
        case MessageType::TEXT_MESSAGE_RESPONSE:
            return "TEXT_MESSAGE_RESPONSE";
        case MessageType::DISCONNECT:
            return "DISCONNECT";
        case MessageType::DISCONNECT_RESPONSE:
            return "DISCONNECT_RESPONSE";
    }

    throw std::runtime_error("Unknown message type");
}

MessageType Message::fromStringToMessageType(const std::string& type)
{
    if (type == "NONE") return MessageType::NONE;
    if (type == "CONNECT") return MessageType::CONNECT;
    if (type == "CONNECT_RESPONSE") return MessageType::CONNECT_RESPONSE;
    if (type == "PEER_LIST") return MessageType::PEER_LIST;
    if (type == "PEER_LIST_RESPONSE") return MessageType::PEER_LIST_RESPONSE;
    if (type == "TEXT_MESSAGE") return MessageType::TEXT_MESSAGE;
    if (type == "TEXT_MESSAGE_RESPONSE") return MessageType::TEXT_MESSAGE_RESPONSE;
    if (type == "DISCONNECT") return MessageType::DISCONNECT;
    if (type == "DISCONNECT_RESPONSE") return MessageType::DISCONNECT_RESPONSE;

    throw std::runtime_error("Unknown message type");
}

void SecretMessage::serialize(json& jData) const
{
    (void)jData;
    throw std::logic_error(
        "This method is not accessible. SecretMessage requires session key for serialization");
}

SecretMessage::SecretMessage() : Message(), signature() {}

SecretMessage::SecretMessage(MessageType type,
                             const peer::UserPeer& from,
                             const peer::UserPeer& to,
                             uint64_t timestamp,
                             const crypto::Bytes& signature)
    : Message(type, from, to, timestamp), signature(signature)
{
}

SecretMessage::SecretMessage(const json& jData, std::shared_ptr<crypto::ICrypto> crypto)
    : Message(jData)
{
    signature = crypto->stringToKey(jData["signature"].get<std::string>());
}
}  // namespace message