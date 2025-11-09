#pragma once

#include <memory>

#include "IConfig.hpp"
#include "ICrypto.hpp"
#include "JsonFile.hpp"

class JsonConfig : public IConfig
{
private:
    JsonFile<std::string> jsonFile;
    std::shared_ptr<ICrypto> crypto;

protected:
    std::map<ConfigField, std::string> data;

public:
    JsonConfig(const std::string& path, std::shared_ptr<ICrypto> crypto);

    std::string get(ConfigField key) const override;
    void update(ConfigField key, const std::string& value) override;
    void loadTrustedPeerList(std::vector<std::string>& trustedPeers) override;

    bool isValid() const override;
    void generatedDefaultConfig() override;
};