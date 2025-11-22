#include "PeerDB.hpp"

namespace peer
{
PeerDB::PeerDB(const std::shared_ptr<db::DBFile>& db) : db(db) {}

void PeerDB::init()
{
    std::lock_guard<std::mutex> lock(mutex);
    db->open();

    db->exec(R"(
        CREATE TABLE IF NOT EXISTS peers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            host TEXT NOT NULL,
            port INTEGER NOT NULL,
            public_key TEXT NOT NULL UNIQUE
        );
    )");
    db->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_peers_public_key ON peers(public_key);
    )");
}

void PeerDB::getAllPeers(std::vector<UserPeer>& peers)
{
    std::lock_guard<std::mutex> lock(mutex);

    db->select("SELECT host, port, public_key FROM peers;",
               [&](const std::vector<std::string>& row)
               {
                   if (row.size() == 3) peers.emplace_back(row[0], std::stoi(row[1]), row[2]);
               });
}

void PeerDB::addPeer(const UserPeer& peer)
{
    std::lock_guard<std::mutex> lock(mutex);

    bool found = false;
    db->select("SELECT id FROM peers WHERE public_key='" + peer.publicKey + "' LIMIT 1;",
               [&](const std::vector<std::string>& row)
               {
                   if (!row.empty()) found = true;
               });

    if (!found)
    {
        db->executePrepared("INSERT INTO peers(host, port, public_key) VALUES (?,?,?);",
                            { peer.host, std::to_string(peer.port), peer.publicKey });
    }
}

bool PeerDB::findPublicKeyByUserHost(const UserHost& host, std::string& publicKey)
{
    std::lock_guard<std::mutex> lock(mutex);

    db->select("SELECT public_key FROM peers WHERE host='" + host.host +
                   "' AND port=" + std::to_string(host.port) + " LIMIT 1;",
               [&](const std::vector<std::string>& row)
               {
                   if (!row.empty()) publicKey = row[0];
               });

    return !publicKey.empty();
}

}  // namespace peer