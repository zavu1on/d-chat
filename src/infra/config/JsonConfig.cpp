#include "JsonConfig.hpp"

#include <iostream>

#include "SocketClient.hpp"

JsonConfig::JsonConfig(const std::string& path) : jsonFile(path)
{
    json jData = jsonFile.read();

    for (json::iterator iter = jData.begin(); iter != jData.end(); ++iter)
    {
        std::string key = iter.key();
        std::string value = iter.value().get<std::string>();

        data[IConfig::stringToConfigField(key)] = value;
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

bool JsonConfig::isValid() const
{
    for (const auto& key : CONFIG_FIELDS)
    {
        if (!data.count(key)) return false;
    }

    return true;
}

void JsonConfig::generatedDefaultConfig()
{
    json jData;

    jData["host"] = "127.0.0.1";
    jData["port"] = std::to_string(network::SocketClient::findFreePort());
    jData["public_key"] = "public_key_secret_123";    // todo gen
    jData["private_key"] = "private_key_secret_123";  // todo gen
    jData["name"] = jData["host"];

    std::map<std::string, std::string> writeData;
    for (const auto& [key, value] : data)
    {
        writeData[IConfig::configFieldToString(key)] = value;
    }

    jsonFile.write(writeData);
}
