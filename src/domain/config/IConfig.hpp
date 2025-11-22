#pragma once

#include <array>
#include <string>
#include <vector>

namespace config
{

enum class ConfigField
{
    HOST,
    PORT,
    PUBLIC_KEY,
    PRIVATE_KEY,
};

const std::array<ConfigField, 4> CONFIG_FIELDS = {
    ConfigField::HOST, ConfigField::PORT, ConfigField::PUBLIC_KEY, ConfigField::PRIVATE_KEY
};

class IConfig
{
public:
    virtual ~IConfig() = default;

    virtual std::string get(ConfigField key) const = 0;
    virtual void loadTrustedPeerList(std::vector<std::string>& trustedPeers) = 0;

    virtual bool isValid() = 0;
    virtual void generatedDefaultConfig() = 0;

    static std::string configFieldToString(ConfigField key);
    static ConfigField stringToConfigField(const std::string& key);
};
}  // namespace config