#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace blockchain
{
using json = nlohmann::json;

class Block
{
public:
    std::string hash;
    std::string previousHash;
    std::string payloadHash;
    std::string authorPublicKey;
    std::string signature;
    uint64_t timestamp = 0;

    Block();
    Block(std::string hash,
          std::string previousHash,
          std::string payloadHash,
          std::string authorPublicKey,
          std::string signature,
          uint64_t timestamp);
    Block(const json& jData);

    std::string toStringForHash() const;
    void computeHash();

    json toJson() const;
};
}  // namespace blockchain