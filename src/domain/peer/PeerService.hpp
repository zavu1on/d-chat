#pragma once

#include <vector>

#include "UserPeer.hpp"

class PeerService
{
private:
    std::vector<UserHost> hosts;
    std::vector<UserPeer> peers;

public:
    PeerService(std::vector<std::string>& hosts);

    const std::vector<UserHost>& getHosts() const;
    const std::vector<UserPeer>& getPeers() const;
    const UserPeer findPeer(const UserHost& host);
    void addPeer(const UserPeer& peer);
    void removePeer(const UserPeer& peer);
};