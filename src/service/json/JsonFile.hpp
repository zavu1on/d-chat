#pragma once

#include <fstream>
#include <iomanip>
#include <map>
#include <nlohmann/json.hpp>

namespace json
{
using json = nlohmann::json;

template <typename T = std::string>
class JsonFile
{
protected:
    std::string path;

public:
    JsonFile(const std::string& path) : path(path) {};

    json read()
    {
        std::ifstream file(path);
        if (!file.is_open()) throw std::runtime_error("Cannot open file");

        try
        {
            return json::parse(file);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Error reading JSON file: " + std::string(e.what()));
        }
    };

    void write(const std::map<std::string, T>& data)
    {
        json jData;
        for (const auto& [key, value] : data) jData[key] = value;

        writeJson(jData);
    };

    void writeJson(const json& jData)
    {
        std::ofstream file(path);
        if (!file.is_open()) throw std::runtime_error("Cannot open file for writing: " + path);

        try
        {
            file << std::setw(4) << jData;
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Error writing JSON file: " + std::string(e.what()));
        }
    }

    std::string getPath() const { return path; };
};
}  // namespace json