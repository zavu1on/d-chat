#pragma once
#include <cstdint>
#include <string>


namespace blockchain
{

struct Block
{
    std::string previousHash;  // hex
    std::string payloadHash;   // hex (we store only hash)
    std::string authorPubKey;  // PEM string (hex not required)
    std::string signature;     // hex
    uint64_t timestamp = 0;
    std::string hash;  // block's own hash (hex)

    std::string toStringForHash() const;  // canonical serialization for hashing
    void computeHash();                   // compute hash via OpenSSL SHA256
};

}  // namespace blockchain
