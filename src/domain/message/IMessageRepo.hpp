#pragma once
#include <string>

#include "TextMessage.hpp"

namespace message
{
class IMessageRepo
{
public:
    virtual ~IMessageRepo() = default;

    virtual void init() = 0;
    virtual void findChatMessages(const std::string& peerAPublicKey,
                                  const std::string& peerBPublicKey,
                                  std::vector<TextMessage>& messages) = 0;
    virtual bool findBlockHashByMessageId(const std::string& messageId, std::string& blockHash) = 0;
    virtual bool insertSecretMessage(const TextMessage& message,
                                     const std::string& messageDump,
                                     const std::string& blockHash) = 0;
    virtual bool removeMessageByBlockHash(const std::string& blockHash) = 0;
};
}  // namespace message