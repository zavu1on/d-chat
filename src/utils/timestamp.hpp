#pragma once
#include <chrono>

namespace utils
{
inline uint64_t getTimestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

inline std::string timestampToString(uint64_t timestamp)
{
    auto timePoint = std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp));
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm* localTime = std::localtime(&time);

    char timeBuffer[64];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", localTime);
    return std::string(timeBuffer);
}
}  // namespace utils