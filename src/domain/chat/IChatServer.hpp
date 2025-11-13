#pragma once

namespace network
{
class IChatServer
{
public:
    virtual ~IChatServer() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
};
}  // namespace network