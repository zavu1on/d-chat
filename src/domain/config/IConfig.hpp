#pragma once

#include <array>
#include <string>
#include <vector>

enum class ConfigField
{
    HOST,
    PORT,
    PUBLIC_KEY,
    PRIVATE_KEY,
    NAME,
};

const std::array<ConfigField, 5> CONFIG_FIELDS = {
    ConfigField::HOST, ConfigField::PORT, ConfigField::PUBLIC_KEY, ConfigField::PRIVATE_KEY, ConfigField::NAME
};

class IConfig
{
public:
    virtual ~IConfig() = default;

    virtual std::string get(ConfigField key) const = 0;
    virtual void update(ConfigField key, const std::string& value) = 0;
    virtual void loadTrustedPeerList(std::vector<std::string>& trustedPeers) = 0;

    virtual bool isValid() const = 0;
    virtual void generatedDefaultConfig() = 0;

    static std::string configFieldToString(ConfigField key);
    static ConfigField stringToConfigField(const std::string& key);
};