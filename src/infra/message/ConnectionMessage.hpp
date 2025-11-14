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
    ConnectionMessage(const json& jData);

    void serialize(json& jData) const override;
};

struct ConnectionMessageResponsePayload
{
    unsigned int peersToReceive;
};

class ConnectionMessageResponse : public Message
{
protected:
    ConnectionMessageResponsePayload payload;

public:
    ConnectionMessageResponse();
    ConnectionMessageResponse(const peer::UserPeer& from,
                              const peer::UserPeer& to,
                              uint64_t timestamp,
                              unsigned int peersToReceive);
    ConnectionMessageResponse(const json& jData);

    void serialize(json& jData) const override;
    const ConnectionMessageResponsePayload& getPayload() const;
};
}  // namespace message