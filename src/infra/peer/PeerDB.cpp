#include "PeerDB.hpp"

namespace peer
{
PeerDB::PeerDB(const std::shared_ptr<db::DBFile>& db) : db(db) {}

void PeerDB::init()
{
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
    db->select("SELECT host, port, public_key FROM peers;",
               [&peers](const std::vector<std::string>& row)
               {
                   if (row.size() == 3) peers.emplace_back(row[0], std::stoi(row[1]), row[2]);
               });
}

void PeerDB::addPeer(const UserPeer& peer)
{
    bool found = false;
    db->selectPrepared("SELECT id FROM peers WHERE public_key=? LIMIT 1;",
                       { peer.publicKey },
                       [&found](const std::vector<std::string>& row)
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
    db->selectPrepared("SELECT public_key FROM peers WHERE host=? AND port=? LIMIT 1;",
                       { host.host, std::to_string(host.port) },
                       [&publicKey](const std::vector<std::string>& row)
                       {
                           if (!row.empty()) publicKey = row[0];
                       });

    return !publicKey.empty();
}
}  // namespace peer