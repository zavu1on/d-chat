#pragma once

#include "Message.hpp"

class ConnectMessage : public Message
{
protected:
    static void deserializeBasic(const json& json, ConnectMessage& message);

public:
    ConnectMessage();
    ConnectMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);

    void serialize(json& json) const override;
    static ConnectMessage fromJson(const json& json);
};

class ConnectResponseMessage : public ConnectMessage
{
public:
    ConnectResponseMessage();
    ConnectResponseMessage(const UserPeer& from, const UserPeer& to, uint64_t timestamp);

    void serialize(json& json) const override;
    static ConnectResponseMessage fromJson(const json& json);
};