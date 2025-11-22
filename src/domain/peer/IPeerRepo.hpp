#pragma once
#include "UserPeer.hpp"

namespace peer
{
class IPeerRepo
{
public:
    virtual ~IPeerRepo() = default;
    virtual void init() = 0;
    virtual void getAllPeers(std::vector<UserPeer>& peers) = 0;
    virtual void addPeer(const UserPeer& peer) = 0;
    virtual bool findPublicKeyByUserHost(const UserHost& host, std::string& publicKey) = 0;
};
}  // namespace peer
