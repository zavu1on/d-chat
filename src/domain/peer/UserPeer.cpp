#include "UserPeer.hpp"

namespace peer
{
UserHost::UserHost() : host(""), port(0) {}

UserHost::UserHost(const std::string& host, unsigned short port) : host(host), port(port) {}

UserPeer::UserPeer() : UserHost(), publicKey("") {}

UserPeer::UserPeer(const std::string& host, unsigned short port, const std::string& publicKey)
    : UserHost(host, port), publicKey(publicKey)
{
}

UserPeer::UserPeer(const json& jData)
    : UserHost(jData["host"].get<std::string>(), jData["port"].get<unsigned short>())
{
    publicKey = jData["public_key"].get<std::string>();
}

json UserPeer::toJson() const
{
    json jData;
    jData["port"] = port;
    jData["host"] = host;
    jData["public_key"] = publicKey;
    return jData;
}

bool UserPeer::operator==(const UserPeer& other) const
{
    return port == other.port && host == other.host && publicKey == other.publicKey;
}
}  // namespace peer
