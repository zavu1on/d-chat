#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "BlockchainService.hpp"
#include "DBFile.hpp"
#include "IConfig.hpp"
#include "ICrypto.hpp"
#include "IMessageRepo.hpp"

namespace message
{
class MessageDB : public IMessageRepo
{
private:
    std::shared_ptr<db::DBFile> db;
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::mutex mutex;

public:
    MessageDB(const std::shared_ptr<db::DBFile>& db,
              const std::shared_ptr<config::IConfig>& config,
              const std::shared_ptr<crypto::ICrypto>& crypto);

    void init() override;
    void findChatMessages(const std::string& peerAPublicKey,
                          const std::string& peerBPublicKey,
                          std::vector<TextMessage>& messages) override;
    bool findBlockHashByMessageId(const std::string& messageId, std::string& blockHash) override;
    bool insertSecretMessage(const TextMessage& message,
                             const std::string& messageDump,
                             const std::string& blockHash) override;
    bool removeMessageByBlockHash(const std::string& blockHash) override;
};
}  // namespace message