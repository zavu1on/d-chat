#pragma once

#include <nlohmann/json.hpp>

#include "Message.hpp"

namespace message
{

using json = nlohmann::json;

struct ConnectionMessagePayload
{
    std::string lastBlockHash;
};

class ConnectionMessage : public Message
{
protected:
    ConnectionMessagePayload payload;

public:
    ConnectionMessage();
    ConnectionMessage(const std::string& id,
                      const peer::UserPeer& from,
                      const peer::UserPeer& to,
                      uint64_t timestamp,
                      const std::string& lastHash);
    ConnectionMessage(const json& jData);

    void serialize(json& jData) const override;
    const ConnectionMessagePayload& getPayload() const;

    static ConnectionMessage create(const peer::UserPeer& from,
                                    const peer::UserPeer& to,
                                    const std::string& lastHash);
};

struct ConnectionMessageResponsePayload
{
    unsigned int peersToReceive;
    unsigned int missingBlocksCount;
};

class ConnectionMessageResponse : public Message
{
protected:
    ConnectionMessageResponsePayload payload;

public:
    ConnectionMessageResponse();
    ConnectionMessageResponse(const std::string& id,
                              const peer::UserPeer& from,
                              const peer::UserPeer& to,
                              uint64_t timestamp,
                              unsigned int peersToReceive,
                              unsigned int missingBlocksCount);
    ConnectionMessageResponse(const json& jData);

    void serialize(json& jData) const override;
    const ConnectionMessageResponsePayload& getPayload() const;

    static ConnectionMessageResponse create(const peer::UserPeer& from,
                                            const peer::UserPeer& to,
                                            unsigned int peersToReceive,
                                            unsigned int missingBlocksCount);
};
}  // namespace message