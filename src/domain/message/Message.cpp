#include "Message.hpp"

#include <stdexcept>

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

Message::Message(MessageType type, const UserPeer& from, const UserPeer& to, uint64_t timestamp)
    : type(type), from(from), to(to), timestamp(timestamp)
{
}

Message::Message(const json& json)
{
    from = UserPeer(json["from"]);
    to = UserPeer(json["to"]);
    timestamp = json["timestamp"].get<uint64_t>();
    type = Message::fromStringToMessageType(json["type"].get<std::string>());
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
        case MessageType::SEND_MESSAGE:
            return "SEND_MESSAGE";
        case MessageType::SEND_MESSAGE_RESPONSE:
            return "SEND_MESSAGE_RESPONSE";
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
    if (type == "SEND_MESSAGE") return MessageType::SEND_MESSAGE;
    if (type == "SEND_MESSAGE_RESPONSE") return MessageType::SEND_MESSAGE_RESPONSE;
    if (type == "DISCONNECT") return MessageType::DISCONNECT;
    if (type == "DISCONNECT_RESPONSE") return MessageType::DISCONNECT_RESPONSE;

    throw std::runtime_error("Unknown message type");
}

void SecretMessage::serialize(json& json) const
{
    (void)json;
    throw std::logic_error(
        "This method is not accessible. SecretMessage requires session key for serialization");
}

SecretMessage::SecretMessage() : Message(), signature() {}

SecretMessage::SecretMessage(MessageType type,
                             const UserPeer& from,
                             const UserPeer& to,
                             uint64_t timestamp,
                             const Bytes& signature)
    : Message(type, from, to, timestamp), signature(signature)
{
}

SecretMessage::SecretMessage(const json& json, std::shared_ptr<ICrypto> crypto) : Message(json)
{
    signature = crypto->stringToKey(json["signature"].get<std::string>());
}
