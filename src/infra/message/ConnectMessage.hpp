#pragma once

#include "Message.hpp"

class ConnectMessage : public Message
{
public:
    ConnectMessage();
    ConnectMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);
    ConnectMessage(const json& json);

    void serialize(json& json) const override;
};

class ConnectResponseMessage : public Message
{
public:
    ConnectResponseMessage();
    ConnectResponseMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);
    ConnectResponseMessage(const json& json);

    void serialize(json& json) const override;
};