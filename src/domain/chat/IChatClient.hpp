#pragma once

#include "Message.hpp"

namespace network
{
class IChatClient
{
public:
    virtual ~IChatClient() = default;

    virtual void connectToAllPeers() = 0;
    virtual void sendMessage(const message::Message& message, bool withSecret) = 0;
    virtual void disconnect() = 0;
};
}  // namespace network