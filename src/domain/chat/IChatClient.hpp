#pragma once

#include "Message.hpp"

class IChatClient
{
public:
    virtual ~IChatClient() = default;

    virtual void sendMessage(const Message& message, bool withSecret = true) = 0;
};