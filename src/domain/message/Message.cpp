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
    if (type == "DISCONNECT") return MessageType::DISCONNECT;
    if (type == "DISCONNECT_RESPONSE") return MessageType::DISCONNECT_RESPONSE;

    throw std::runtime_error("Unknown message type");
}
