#include "IConfig.hpp"

#include <stdexcept>

namespace config
{
std::string IConfig::configFieldToString(ConfigField key)
{
    switch (key)
    {
        case ConfigField::HOST:
            return "host";
        case ConfigField::PORT:
            return "port";
        case ConfigField::PUBLIC_KEY:
            return "public_key";
        case ConfigField::PRIVATE_KEY:
            return "private_key";
    }

    throw std::runtime_error("Unknown config field");
}

ConfigField IConfig::stringToConfigField(const std::string& key)
{
    if (key == "host")
        return ConfigField::HOST;
    else if (key == "port")
        return ConfigField::PORT;
    else if (key == "public_key")
        return ConfigField::PUBLIC_KEY;
    else if (key == "private_key")
        return ConfigField::PRIVATE_KEY;

    throw std::runtime_error("Unknown config field");
}
}  // namespace config