#pragma once

#include "IConfig.hpp"
#include "JsonFile.hpp"

class JsonConfig : public IConfig
{
private:
    JsonFile<std::string> jsonFile;

protected:
    std::map<ConfigField, std::string> data;

public:
    JsonConfig(const std::string& path);

    std::string get(ConfigField key) const override;
    void update(ConfigField key, const std::string& value) override;
};