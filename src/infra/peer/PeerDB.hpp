#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "DBFile.hpp"
#include "IPeerRepo.hpp"

namespace peer
{
class PeerDB : public IPeerRepo
{
private:
    std::shared_ptr<db::DBFile> db;
    std::mutex mutex;

public:
    PeerDB(const std::shared_ptr<db::DBFile>& db);

    void init() override;
    void getAllPeers(std::vector<UserPeer>& peers) override;
    void addPeer(const UserPeer& peer) override;
    bool findPublicKeyByUserHost(const UserHost& host, std::string& publicKey) override;
};
}  // namespace peer