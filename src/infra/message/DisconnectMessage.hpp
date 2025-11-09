#pragma once

#include "Message.hpp"

class DisconnectMessage : public Message
{
protected:
    static void deserializeBasic(const json& json, DisconnectMessage& message);

public:
    DisconnectMessage();
    DisconnectMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);

    void serialize(json& json) const override;
    static DisconnectMessage fromJson(const json& json);
};

class DisconnectResponseMessage : public DisconnectMessage
{
public:
    DisconnectResponseMessage();
    DisconnectResponseMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);

    static DisconnectResponseMessage fromJson(const json& json);
};