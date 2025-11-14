#include "GlobalState.hpp"

namespace config
{

GlobalState::GlobalState() : peersToReceive(0) {}

GlobalState& GlobalState::getInstance()
{
    static GlobalState instance;
    return instance;
}

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

}  // namespace config
