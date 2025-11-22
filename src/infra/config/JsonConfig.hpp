#pragma once

#include <memory>
#include <nlohmann/json.hpp>

#include "IConfig.hpp"
#include "ICrypto.hpp"
#include "JsonFile.hpp"

namespace config
{
class JsonConfig : public IConfig
{
private:
    json::JsonFile<std::string> jsonFile;
    std::shared_ptr<crypto::ICrypto> crypto;

protected:
    std::map<ConfigField, std::string> data;

public:
    JsonConfig(const std::string& path, const std::shared_ptr<crypto::ICrypto>& crypto);

    std::string get(ConfigField key) const override;
    void loadTrustedPeerList(std::vector<std::string>& trustedPeers) override;

    bool isValid() override;
    void generatedDefaultConfig() override;
};
}  // namespace config