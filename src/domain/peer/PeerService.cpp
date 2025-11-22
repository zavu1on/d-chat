#include "PeerService.hpp"

#include <algorithm>
#include <iostream>
#include <mutex>

namespace peer
{
PeerService::PeerService(std::vector<std::string>& hosts,
                         const std::shared_ptr<IPeerRepo>& peerRepo)
    : peerRepo(peerRepo)
{
    for (const std::string& trustedPeer : hosts)
    {
        size_t sepPos = trustedPeer.find(':');
        if (sepPos == std::string::npos)
            throw std::runtime_error("Invalid peer format: " + trustedPeer);

        std::string host = trustedPeer.substr(0, sepPos);
        unsigned short port = std::stoul(trustedPeer.substr(sepPos + 1));

        this->hosts.emplace_back(host, port);
    }
}

const std::vector<UserHost>& PeerService::getHosts() const { return hosts; }

std::vector<UserPeer> PeerService::getPeers() const
{
    std::shared_lock lock(mutex);
    return peers;
}

int PeerService::getPeersCount() const
{
    std::shared_lock lock(mutex);
    return peers.size();
}

UserPeer PeerService::getPeer(size_t index) const
{
    std::shared_lock lock(mutex);
    return peers.at(index);
}

UserPeer PeerService::findPeer(const UserHost& host) const
{
    std::shared_lock lock(mutex);
    auto it = std::find_if(peers.begin(),
                           peers.end(),
                           [&host](const UserPeer& peer)
                           { return peer.host == host.host && peer.port == host.port; });

    if (it != peers.end())
        return *it;
    else
        throw std::range_error("Peer not found");
}

void PeerService::addPeer(const UserPeer& peer)
{
    std::unique_lock lock(mutex);
    auto it = std::find(peers.begin(), peers.end(), peer);
    if (it == peers.end()) peers.push_back(peer);
}

void PeerService::removePeer(const UserPeer& peer)
{
    std::unique_lock lock(mutex);
    auto it = std::find(peers.begin(), peers.end(), peer);
    if (it != peers.end()) peers.erase(it);
}

void PeerService::getAllChatPeers(std::vector<UserPeer>& peers) { peerRepo->getAllPeers(peers); }

void PeerService::addChatPeer(const UserPeer& peer) { peerRepo->addPeer(peer); }

bool PeerService::findPublicKeyByUserHost(const UserHost& host, std::string& publicKey)
{
    return peerRepo->findPublicKeyByUserHost(host, publicKey);
}
}  // namespace peer
