#include "GlobalState.hpp"

namespace config
{
GlobalState GlobalState::instance;

GlobalState::GlobalState() : peersToReceive(0) {}

GlobalState& GlobalState::getInstance() { return instance; }

unsigned int GlobalState::getPeersToReceive() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return peersToReceive;
}

void GlobalState::setPeersToReceive(unsigned int value)
{
    std::lock_guard<std::mutex> lock(mutex);
    peersToReceive = value;
}

unsigned int GlobalState::getMissingBlocksCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return missingBlocksCount;
}

void GlobalState::setMissingBlocksCount(unsigned int value)
{
    std::lock_guard<std::mutex> lock(mutex);
    missingBlocksCount = value;
}
}  // namespace config
