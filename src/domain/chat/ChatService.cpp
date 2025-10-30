#include "ChatService.hpp"

#include <iostream>

std::string ChatService::handleIncoming(const std::string& message)
{
    std::cout << "[INFO] Incoming message from client: " << message << std::endl;
    return std::string("Response: ") + message;
}

void ChatService::handleOutgoing(const std::string& message)
{
    // some logic with crypto and db
    std::cout << "[INFO] Outgoing message from server: " << message << std::endl;
}
