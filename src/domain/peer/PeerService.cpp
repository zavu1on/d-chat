#include "PeerService.hpp"

#include <iostream>

PeerService::PeerService(std::vector<std::string>& hosts)
{
    for (const std::string& trustedPeer : hosts)
    {
        size_t sepPos = trustedPeer.find(':');
        if (sepPos == std::string::npos)
            throw std::runtime_error("Invalid peer format: " + trustedPeer);

        std::string host = trustedPeer.substr(0, sepPos);
        unsigned short port = std::stoul(trustedPeer.substr(sepPos + 1));

        this->hosts.push_back(UserHost(host, port));
    }
}

const std::vector<UserHost>& PeerService::getHosts() const { return hosts; }

const std::vector<UserPeer>& PeerService::getPeers() const { return peers; }

void PeerService::addPeer(const UserPeer& peer)
{
    auto el = std::find(peers.begin(), peers.end(), peer);
    if (el == peers.end()) peers.push_back(peer);
}

void PeerService::removePeer(const UserPeer& peer)
{
    auto el = std::find(peers.begin(), peers.end(), peer);
    if (el != peers.end()) peers.erase(el);
}
