#include "JsonConfig.hpp"

#include <stdexcept>
#include <utility>

#include "SocketClient.hpp"

JsonConfig::JsonConfig(const std::string& path, std::shared_ptr<ICrypto> crypto) : jsonFile(path), crypto(crypto)
{
    json jData = jsonFile.read();

    for (json::iterator iter = jData.begin(); iter != jData.end(); ++iter)
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

void JsonConfig::update(ConfigField key, const std::string& value)
{
    data[key] = value;

    std::map<std::string, std::string> writeData;
    for (const auto& [k, v] : data)
    {
        writeData[IConfig::configFieldToString(k)] = v;
    }

    jsonFile.write(writeData);
}

void JsonConfig::loadTrustedPeerList(std::vector<std::string>& trustedPeers)
{
    trustedPeers.clear();
    json jData = jsonFile.read();

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

bool JsonConfig::isValid() const
{
    for (const auto& key : CONFIG_FIELDS)
        if (!data.count(key)) return false;

    return true;
}

void JsonConfig::generatedDefaultConfig()
{
    json jData;

    jData["host"] = "127.0.0.1";
    jData["port"] = std::to_string(SocketClient::findFreePort());
    jData["public_key"] = crypto->keyToString(crypto->generateSecret());
    jData["private_key"] = crypto->keyToString(crypto->generateSecret());
    jData["name"] = jData["host"];

    std::map<std::string, std::string> writeData;
    for (json::iterator iter = jData.begin(); iter != jData.end(); ++iter)
    {
        std::string key = iter.key();
        std::string value = iter.value().get<std::string>();

        writeData[key] = value;
    }

    jsonFile.write(writeData);
}
