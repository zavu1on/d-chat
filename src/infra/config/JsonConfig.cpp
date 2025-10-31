#include "JsonConfig.hpp"

#include <iostream>

JsonConfig::JsonConfig(const std::string& path) : jsonFile(path)
{
    json jData = jsonFile.read();

    for (json::iterator it = jData.begin(); it != jData.end(); ++it)
    {
        std::string key = it.key();
        std::string value = it.value().get<std::string>();

        if (key == "host")
            data[ConfigField::HOST] = value;
        else if (key == "port")
            data[ConfigField::PORT] = value;
        else if (key == "public_key")
            data[ConfigField::PUBLIC_KEY] = value;
        else if (key == "private_key")
            data[ConfigField::PRIVATE_KEY] = value;
    }

    if (!data.count(ConfigField::HOST) || !data.count(ConfigField::PORT) ||
        !data.count(ConfigField::PUBLIC_KEY) || !data.count(ConfigField::PRIVATE_KEY))
        throw std::runtime_error("Invalid config file");
}

std::string JsonConfig::get(ConfigField key) const { return data.at(key); }

void JsonConfig::update(ConfigField key, const std::string& value)
{
    data[key] = value;

    std::map<std::string, std::string> writeData;
    for (const auto& [key, value] : data)
    {
        writeData[IConfig::configFieldToString(key)] = value;
    }

    jsonFile.write(writeData);
}
