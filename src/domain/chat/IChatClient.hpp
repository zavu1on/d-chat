#pragma once

#include <string>

class IChatClient
{
public:
    virtual ~IChatClient() = default;

    // todo change message to VO
    virtual std::string sendMessage(const std::string& message) = 0;
};