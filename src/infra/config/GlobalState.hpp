#pragma once

#include <mutex>

namespace config
{
class GlobalState
{
private:
    unsigned int peersToReceive;
    mutable std::mutex mutex;

    GlobalState();

public:
    GlobalState(const GlobalState&) = delete;
    GlobalState& operator=(const GlobalState&) = delete;

    static GlobalState& getInstance();

    unsigned int getPeersToReceive() const;
    void setPeersToReceive(unsigned int value);
};
}  // namespace config
