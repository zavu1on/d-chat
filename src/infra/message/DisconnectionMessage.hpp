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
    DisconnectionMessage(const std::string& id,
                         const peer::UserPeer& from,
                         const peer::UserPeer& to,
                         uint64_t timestamp);
    DisconnectionMessage(const json& jData);

    void serialize(json& jData) const override;

    static DisconnectionMessage create(const peer::UserPeer& from, const peer::UserPeer& to);
};

class DisconnectionMessageResponse : public Message
{
public:
    DisconnectionMessageResponse();
    DisconnectionMessageResponse(const std::string& id,
                                 const peer::UserPeer& from,
                                 const peer::UserPeer& to,
                                 uint64_t timestamp);
    DisconnectionMessageResponse(const json& jData);

    void serialize(json& jData) const override;

    static DisconnectionMessageResponse create(const peer::UserPeer& from,
                                               const peer::UserPeer& to);
};
}  // namespace message