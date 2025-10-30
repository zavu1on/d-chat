#pragma once

#include <string>

class ChatService
{
private:
    // todo dependencies (repositories, other services)

public:
    explicit ChatService() {}

    // todo change message to VO
    std::string handleIncoming(const std::string& message);

    // todo change message to VO
    void handleOutgoing(const std::string& message);
};
