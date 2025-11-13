#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace peer
{
using json = nlohmann::json;

class UserHost
{
public:
    std::string host;
    unsigned short port;

    UserHost();
    UserHost(const std::string& host, unsigned short port);
};

class UserPeer : public UserHost
{
public:
    std::string publicKey;

    UserPeer();
    UserPeer(const std::string& host, unsigned short port, const std::string& publicKey);
    UserPeer(const json& jData);
    json toJson() const;

    bool operator==(const UserPeer& other) const;
};
}  // namespace peer