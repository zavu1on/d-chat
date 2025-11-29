#pragma once

#include <memory>
#include <shared_mutex>
#include <vector>

#include "IPeerRepo.hpp"
#include "UserPeer.hpp"

namespace peer
{
class PeerService
{
private:
    std::vector<UserHost> hosts;
    std::vector<UserPeer> peers;
    mutable std::shared_mutex mutex;  // allows call from const methods
    std::shared_ptr<peer::IPeerRepo> peerRepo;

public:
    PeerService(std::vector<std::string>& hosts, const std::shared_ptr<peer::IPeerRepo>& peerRepo);

    const std::vector<UserHost>& getHosts() const;

    std::vector<UserPeer> getPeers() const;
    int getPeersCount() const;
    UserPeer getPeer(size_t index) const;
    UserPeer findPeer(const UserHost& host) const;
    void addPeer(const UserPeer& peer);
    void removePeer(const UserPeer& peer);

    void getAllChatPeers(std::vector<UserPeer>& peers);
    void addChatPeer(const UserPeer& peer);
    bool findPublicKeyByUserHost(const UserHost& host, std::string& publicKey);
};
}  // namespace peer
