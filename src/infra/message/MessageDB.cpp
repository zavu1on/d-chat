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
    std::lock_guard<std::mutex> lock(mutex);
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
    std::lock_guard<std::mutex> lock(mutex);

    db->select(
        "SELECT message_json "
        "FROM messages WHERE to_public_key='" +
            peerAPublicKey + "' AND from_public_key='" + peerBPublicKey + "';",
        [&](const std::vector<std::string>& row)
        {
            if (row.size() < 1) return;
            json jData = json::parse(row[0]);
            messages.emplace_back(jData, config->get(config::ConfigField::PRIVATE_KEY), crypto);
        });
    db->select(
        "SELECT message_json "
        "FROM messages WHERE to_public_key='" +
            peerBPublicKey + "' AND from_public_key='" + peerAPublicKey + "';",
        [&](const std::vector<std::string>& row)
        {
            if (row.size() < 1) return;
            json jData = json::parse(row[0]);
            messages.emplace_back(
                jData, config->get(config::ConfigField::PRIVATE_KEY), crypto, true);
        });

    std::sort(messages.begin(),
              messages.end(),
              [](const TextMessage& a, const TextMessage& b)
              { return a.getTimestamp() < b.getTimestamp(); });
}

bool MessageDB::findBlockHashByMessageId(const std::string& messageId, std::string& blockHash)
{
    std::lock_guard<std::mutex> lock(mutex);
    bool found = false;

    db->select("SELECT block_hash FROM messages WHERE message_id='" + messageId + "' LIMIT 1;",
               [&](const std::vector<std::string>& row)
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
    std::lock_guard<std::mutex> lock(mutex);

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
    std::lock_guard<std::mutex> lock(mutex);
    return db->executePrepared("DELETE FROM messages WHERE block_hash=?;", { blockHash });
}

}  // namespace message
