#pragma once
#include "Message.hpp"
#include "UserPeer.hpp"

namespace message
{
using u_int = unsigned int;

struct PeerListMessagePayload
{
    u_int start;
    u_int count;
};

class PeerListMessage : public Message
{
protected:
    PeerListMessagePayload payload;

public:
    PeerListMessage();
    PeerListMessage(const std::string& id,
                    const peer::UserPeer& from,
                    const peer::UserPeer& to,
                    uint64_t timestamp,
                    u_int start,
                    u_int count);
    PeerListMessage(const json& jData);

    void serialize(json& jData) const override;
    const PeerListMessagePayload& getPayload() const;
};

struct PeerListMessageResponsePayload
{
    std::vector<peer::UserPeer> peers;
};

class PeerListMessageResponse : public Message
{
protected:
    PeerListMessageResponsePayload payload;

public:
    PeerListMessageResponse();
    PeerListMessageResponse(const std::string& id,
                            const peer ::UserPeer& from,
                            const peer ::UserPeer& to,
                            uint64_t timestamp,
                            const std::vector<peer ::UserPeer> peers);
    PeerListMessageResponse(const json& jData);

    void serialize(json& jData) const override;
    const PeerListMessageResponsePayload& getPayload() const;
};
}  // namespace message