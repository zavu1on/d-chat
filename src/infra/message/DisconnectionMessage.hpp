#pragma once

#include <nlohmann/json.hpp>

#include "Message.hpp"

namespace message
{
using json = nlohmann::json;

class DisconnectionMessage : public Message
{
public:
    DisconnectionMessage();
    DisconnectionMessage(const peer::UserPeer& from, const peer::UserPeer& to, uint64_t timestamp);
    DisconnectionMessage(const json& json);

    void serialize(json& json) const override;
};

class DisconnectionMessageResponse : public Message
{
public:
    DisconnectionMessageResponse();
    DisconnectionMessageResponse(const peer::UserPeer& from,
                                 const peer::UserPeer& to,
                                 uint64_t timestamp);
    DisconnectionMessageResponse(const json& json);

    void serialize(json& json) const override;
};
}  // namespace message