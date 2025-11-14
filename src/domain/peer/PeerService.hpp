#pragma once

#include <shared_mutex>
#include <vector>

#include "UserPeer.hpp"

namespace peer
{
class PeerService
{
private:
    std::vector<UserHost> hosts;
    std::vector<UserPeer> peers;
    mutable std::shared_mutex mutex;

public:
    PeerService(std::vector<std::string>& hosts);

    const std::vector<UserHost>& getHosts() const;

    std::vector<UserPeer> getPeers() const;
    int getPeersCount() const;
    UserPeer getPeer(size_t index) const;
    UserPeer findPeer(const UserHost& host) const;
    void addPeer(const UserPeer& peer);
    void removePeer(const UserPeer& peer);
};
}  // namespace peer
