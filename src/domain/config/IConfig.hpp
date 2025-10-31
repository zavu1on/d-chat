#pragma once

#include <string>

enum class ConfigField
{
    HOST,
    PORT,
    PUBLIC_KEY,
    PRIVATE_KEY,
};

class IConfig
{
public:
    virtual ~IConfig() = default;

    virtual std::string get(ConfigField key) const = 0;
    virtual void update(ConfigField key, const std::string& value) = 0;

    static std::string configFieldToString(ConfigField key);
};