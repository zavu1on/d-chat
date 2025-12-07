#include "MessageDB.hpp"

namespace message
{

MessageDB::MessageDB(const std::shared_ptr<db::DBFile>& db,
                     const std::shared_ptr<config::IConfig>& config,
                     const std::shared_ptr<crypto::ICrypto>& crypto)
    : db(db), config(config), crypto(crypto)
{
}

void MessageDB::init()
{
    db->open();

    db->exec(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            message_id TEXT NOT NULL UNIQUE,
            to_public_key TEXT NOT NULL,
            from_public_key TEXT NOT NULL,
            timestamp INTEGER NOT NULL,
            message_json TEXT NOT NULL,
            block_hash TEXT NOT NULL
        );
    )");
    db->exec(R"(
        CREATE INDEX IF NOT EXISTS idx_messages_message_id ON messages(message_id);
    )");
}

void MessageDB::findChatMessages(const std::string& peerAPublicKey,
                                 const std::string& peerBPublicKey,
                                 std::vector<TextMessage>& messages)
{
    bool error = false;

    db->selectPrepared(
        "SELECT message_json "
        "FROM messages WHERE to_public_key=? AND from_public_key=?;",
        { peerAPublicKey, peerBPublicKey },
        [&messages, &error, this](const std::vector<std::string>& row)
        {
            if (row.size() < 1) return;
            json jData = json::parse(row[0]);
            try
            {
                messages.emplace_back(
                    jData, config->get(config::ConfigField::PRIVATE_KEY), crypto, false, true);
            }
            catch (const std::exception&)
            {
                error = true;
            }
        });
    db->selectPrepared(
        "SELECT message_json "
        "FROM messages WHERE to_public_key=? AND from_public_key=?;",
        { peerBPublicKey, peerAPublicKey },
        [&messages, &error, this](const std::vector<std::string>& row)
        {
            if (row.size() < 1) return;
            json jData = json::parse(row[0]);
            try
            {
                messages.emplace_back(
                    jData, config->get(config::ConfigField::PRIVATE_KEY), crypto, true, true);
            }
            catch (const std::exception&)
            {
                error = true;
            }
        });

    std::sort(messages.begin(),
              messages.end(),
              [](const TextMessage& a, const TextMessage& b)
              { return a.getTimestamp() < b.getTimestamp(); });

    if (error) throw std::exception();
}

bool MessageDB::findBlockHashByMessageId(const std::string& messageId, std::string& blockHash)
{
    bool found = false;

    db->selectPrepared("SELECT block_hash FROM messages WHERE message_id=? LIMIT 1;",
                       { messageId },
                       [&blockHash, &found](const std::vector<std::string>& row)
                       {
                           if (!row.empty())
                           {
                               blockHash = row[0];
                               found = true;
                           }
                       });

    return found;
}

bool MessageDB::insertSecretMessage(const TextMessage& message,
                                    const std::string& messageDump,
                                    const std::string& blockHash)
{
    return db->executePrepared(
        "INSERT INTO messages(message_id, to_public_key, from_public_key, timestamp, message_json, "
        "block_hash) "
        "VALUES (?,?,?,?,?,?);",
        { message.getId(),
          message.getTo().publicKey,
          message.getFrom().publicKey,
          std::to_string(message.getTimestamp()),
          messageDump,
          blockHash });
}

bool MessageDB::removeMessageByBlockHash(const std::string& blockHash)
{
    return db->executePrepared("DELETE FROM messages WHERE block_hash=?;", { blockHash });
}
}  // namespace message
