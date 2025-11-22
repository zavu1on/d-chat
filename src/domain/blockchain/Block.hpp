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
    std::string hash;             // block's own hash (hex)
    std::string previousHash;     // hex
    std::string payloadHash;      // hex (we store only hash)
    std::string authorPublicKey;  // PEM string (hex not required)
    std::string signature;        // hex
    uint64_t timestamp = 0;

    Block();
    Block(std::string hash,
          std::string previousHash,
          std::string payloadHash,
          std::string authorPublicKey,
          std::string signature,
          uint64_t timestamp);
    Block(const json& jData);

    std::string toStringForHash() const;  // canonical serialization for hashing
    void computeHash();                   // compute hash via OpenSSL SHA256

    json toJson() const;
};
}  // namespace blockchain