#pragma once
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace utils
{

inline std::string toHex(const std::vector<uint8_t>& data)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : data) oss << std::setw(2) << static_cast<int>(b);
    return oss.str();
}

inline std::vector<uint8_t> fromHex(const std::string& hex)
{
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
    {
        uint8_t byte = static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16));
        out.push_back(byte);
    }
    return out;
}
}  // namespace utils