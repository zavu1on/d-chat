#pragma once

#include <mutex>

namespace config
{
class GlobalState
{
private:
    static GlobalState instance;

    unsigned int peersToReceive;
    unsigned int missingBlocksCount;
    mutable std::mutex mutex;

    GlobalState();

public:
    GlobalState(const GlobalState&) = delete;
    GlobalState& operator=(const GlobalState&) = delete;

    static GlobalState& getInstance();

    unsigned int getPeersToReceive() const;
    void setPeersToReceive(unsigned int value);
    unsigned int getMissingBlocksCount() const;
    void setMissingBlocksCount(unsigned int value);
};
}  // namespace config
