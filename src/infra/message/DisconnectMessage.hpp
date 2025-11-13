#pragma once

#include "Message.hpp"

class DisconnectMessage : public Message
{
public:
    DisconnectMessage();
    DisconnectMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);
    DisconnectMessage(const json& json);

    void serialize(json& json) const override;
};

class DisconnectResponseMessage : public Message
{
public:
    DisconnectResponseMessage();
    DisconnectResponseMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);
    DisconnectResponseMessage(const json& json);

    void serialize(json& json) const override;
};