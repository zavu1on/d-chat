#include "IConfig.hpp"

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

    return "";
}
