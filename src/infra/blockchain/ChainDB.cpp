#include "ChainDB.hpp"

#include <iostream>
#include <stdexcept>

namespace blockchain
{

ChainDB::ChainDB(const std::shared_ptr<db::DBFile>& db,
                 const std::shared_ptr<config::IConfig>& config,
                 const std::shared_ptr<crypto::ICrypto>& crypto)
    : db(db), config(config), crypto(crypto)
{
}

void ChainDB::init()
{
    std::lock_guard<std::mutex> lock(mutex);
    db->open();

    db->exec(R"(
        CREATE TABLE IF NOT EXISTS blocks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            hash TEXT NOT NULL UNIQUE,
            previous_hash TEXT NOT NULL,
            payload_hash TEXT NOT NULL,
            author_public_key TEXT NOT NULL,
            signature TEXT NOT NULL,
            timestamp INTEGER NOT NULL
        );
    )");
    db->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_blocks_hash ON blocks(hash);
    )");
}

bool ChainDB::insertBlock(const Block& block)
{
    std::lock_guard<std::mutex> lock(mutex);

    return db->executePrepared(
        "INSERT INTO blocks(hash, previous_hash, payload_hash, author_public_key, signature, "
        "timestamp)"
        "VALUES (?,?,?,?,?,?);",
        {
            block.hash,
            block.previousHash,
            block.payloadHash,
            block.authorPublicKey,
            block.signature,
            std::to_string(block.timestamp),
        });
}

bool ChainDB::findBlockByHash(const std::string& hash, Block& block)
{
    std::lock_guard<std::mutex> lock(mutex);
    bool found = false;

    db->selectPrepared(
        "SELECT hash, previous_hash, payload_hash, author_public_key, signature, timestamp "
        "FROM blocks WHERE hash=? LIMIT 1;",
        { hash },
        [&](const std::vector<std::string>& row)
        {
            if (row.size() < 6) return;
            block.hash = row[0];
            block.previousHash = row[1];
            block.payloadHash = row[2];
            block.authorPublicKey = row[3];
            block.signature = row[4];
            block.timestamp = std::stoull(row[5]);
            found = true;
        });

    return found;
}

bool ChainDB::findBlockIndexByHash(const std::string& hash, u_int& index)
{
    std::lock_guard<std::mutex> lock(mutex);
    bool found = false;

    db->selectPrepared("SELECT id FROM blocks WHERE hash=? LIMIT 1;",
                       { hash },
                       [&](const std::vector<std::string>& row)
                       {
                           if (row.empty()) return;
                           index = std::stoull(row[0]);
                           found = true;
                       });

    return found;
}

void ChainDB::getBlocksByIndexRange(u_int start,
                                    u_int count,
                                    const std::string& lastHash,
                                    std::vector<Block>& outBlocks)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (lastHash == "0")
    {
        db->selectPrepared(
            "SELECT hash, previous_hash, payload_hash, author_public_key, signature, timestamp "
            "FROM blocks ORDER BY id ASC LIMIT ? OFFSET ?;",
            { std::to_string(count), std::to_string(start) },
            [&](const std::vector<std::string>& row)
            {
                if (row.size() < 6) return;
                outBlocks.emplace_back(row[0], row[1], row[2], row[3], row[4], std::stoull(row[5]));
            });
    }
    else
    {
        uint64_t lastBlockIndex = 0;
        bool found = false;

        db->selectPrepared("SELECT id FROM blocks WHERE hash=? LIMIT 1;",
                           { lastHash },
                           [&](const std::vector<std::string>& row)
                           {
                               if (!row.empty())
                               {
                                   lastBlockIndex = std::stoull(row[0]);
                                   found = true;
                               }
                           });

        if (!found) return;

        uint64_t startIndex = lastBlockIndex + 1 + start;

        db->selectPrepared(
            "SELECT hash, previous_hash, payload_hash, author_public_key, signature, timestamp "
            "FROM blocks WHERE id >= ? ORDER BY id ASC LIMIT ?;",
            { std::to_string(startIndex), std::to_string(count) },
            [&](const std::vector<std::string>& row)
            {
                if (row.size() < 6) return;
                outBlocks.emplace_back(row[0], row[1], row[2], row[3], row[4], std::stoull(row[5]));
            });
    }
}

u_int ChainDB::countBlocksAfterHash(const std::string& hash)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (hash == "0")
    {
        u_int count = 0;
        db->select("SELECT COUNT(*) FROM blocks;",
                   [&](const std::vector<std::string>& row)
                   {
                       if (!row.empty()) count = std::stoull(row[0]);
                   });
        return count;
    }

    u_int index = 0;
    bool exists = false;

    db->selectPrepared("SELECT id FROM blocks WHERE hash=? LIMIT 1;",
                       { hash },
                       [&](const std::vector<std::string>& row)
                       {
                           if (row.empty()) return;
                           index = std::stoull(row[0]);
                           exists = true;
                       });

    if (!exists)
    {
        u_int count = 0;
        db->select("SELECT COUNT(*) FROM blocks;",
                   [&](const std::vector<std::string>& row)
                   {
                       if (!row.empty()) count = std::stoull(row[0]);
                   });
        return count;
    }

    u_int missing = 0;
    db->selectPrepared("SELECT COUNT(*) FROM blocks WHERE id > ?;",
                       { std::to_string(index) },
                       [&](const std::vector<std::string>& row)
                       {
                           if (!row.empty()) missing = std::stoull(row[0]);
                       });

    return missing;
}

bool ChainDB::findTip(Block& block)
{
    std::lock_guard<std::mutex> lock(mutex);
    bool found = false;

    db->select(
        "SELECT hash, previous_hash, payload_hash, author_public_key, signature, timestamp "
        "FROM blocks ORDER BY id DESC LIMIT 1;",
        [&](const std::vector<std::string>& row)
        {
            if (row.size() < 6) return;
            block.hash = row[0];
            block.previousHash = row[1];
            block.payloadHash = row[2];
            block.authorPublicKey = row[3];
            block.signature = row[4];
            block.timestamp = std::stoull(row[5]);
            found = true;
        });

    return found;
}

bool ChainDB::findTipIndex(u_int& index)
{
    std::lock_guard<std::mutex> lock(mutex);
    bool found = false;

    db->select("SELECT id FROM blocks ORDER BY id DESC LIMIT 1;",
               [&](const std::vector<std::string>& row)
               {
                   if (row.empty()) return;
                   index = std::stoull(row[0]);
                   found = true;
               });

    return found;
}

bool ChainDB::hasBlock(const std::string& hash)
{
    std::lock_guard<std::mutex> lock(mutex);
    bool exists = false;

    db->selectPrepared("SELECT 1 FROM blocks WHERE hash=? LIMIT 1;",
                       { hash },
                       [&](const std::vector<std::string>& row)
                       {
                           if (!row.empty()) exists = true;
                       });

    return exists;
}

void ChainDB::loadAllBlocks(std::vector<Block>& blocks)
{
    std::lock_guard<std::mutex> lock(mutex);

    db->select(
        "SELECT hash, previous_hash, payload_hash, author_public_key, signature, timestamp "
        "FROM blocks ORDER BY id ASC;",
        [&](const std::vector<std::string>& row)
        {
            if (row.size() < 6) return;
            blocks.emplace_back(row[0], row[1], row[2], row[3], row[4], std::stoull(row[5]));
        });
}

}  // namespace blockchain
