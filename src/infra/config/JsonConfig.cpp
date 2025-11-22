#include "JsonConfig.hpp"

#include <stdexcept>
#include <utility>

#include "SocketClient.hpp"

namespace config
{

JsonConfig::JsonConfig(const std::string& path, const std::shared_ptr<crypto::ICrypto>& crypto)
    : jsonFile(path), crypto(crypto)
{
    json::json jData;
    try
    {
        jData = jsonFile.read();
    }
    catch (const std::exception& error)
    {
        if (std::string(error.what()) == "Cannot open file") generatedDefaultConfig();
        jData = jsonFile.read();
    }

    for (json::json::iterator iter = jData.begin(); iter != jData.end(); ++iter)
    {
        auto key = iter.key();
        auto value = iter.value();

        if (value.is_string())
        {
            data[IConfig::stringToConfigField(key)] = value.get<std::string>();
        }
    }
}

std::string JsonConfig::get(ConfigField key) const { return data.at(key); }

void JsonConfig::loadTrustedPeerList(std::vector<std::string>& trustedPeers)
{
    trustedPeers.clear();
    json::json jData = jsonFile.read();

    if (jData.contains("trustedPeerList") && jData["trustedPeerList"].is_array())
    {
        for (const auto& peer : jData["trustedPeerList"])
        {
            if (peer.is_string())
                trustedPeers.push_back(peer.get<std::string>());
            else
                throw std::runtime_error("Invalid 'trustedPeerList' found in configuration");
        }
    }
    else
        throw std::runtime_error("Missing 'trustedPeerList' in configuration");
}

bool JsonConfig::isValid()
{
    for (const auto& key : CONFIG_FIELDS)
        if (!data.count(key)) return false;

    json::json jData = jsonFile.read();
    if (jData.contains("trustedPeerList") && jData["trustedPeerList"].is_array()) return true;

    return false;
}

void JsonConfig::generatedDefaultConfig()
{
    json::json jData;
    crypto::KeyPair keyPair = crypto->generateKeyPair();

    jData["host"] = "127.0.0.1";
    jData["port"] = std::to_string(network::SocketClient::findFreePort());
    jData["public_key"] = crypto->keyToString(keyPair.publicKey);
    jData["private_key"] = crypto->keyToString(keyPair.privateKey);
    jData["trustedPeerList"] = json::json::array();

    jsonFile.writeJson(jData);
}
}  // namespace config