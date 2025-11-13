#pragma once

#include <nlohmann/json.hpp>

#include "Message.hpp"

namespace message
{

using json = nlohmann::json;

class ConnectionMessage : public Message
{
public:
    ConnectionMessage();
    ConnectionMessage(const peer::UserPeer& from, const peer::UserPeer& to, uint64_t timestamp);
    ConnectionMessage(const json& json);

    void serialize(json& json) const override;
};

class ConnectionMessageResponse : public Message
{
public:
    ConnectionMessageResponse();
    ConnectionMessageResponse(const peer::UserPeer& from,
                              const peer::UserPeer& to,
                              uint64_t timestamp);
    ConnectionMessageResponse(const json& json);

    void serialize(json& json) const override;
};
}  // namespace message