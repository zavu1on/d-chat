#include "Block.hpp"

#include <openssl/sha.h>

#include <sstream>

#include "hex.hpp"

namespace blockchain
{

std::string Block::toStringForHash() const
{
    std::ostringstream oss;
    oss << previousHash << "|" << payloadHash << "|" << authorPubKey << "|" << signature << "|" << timestamp;
    return oss.str();
}

void Block::computeHash()
{
    std::string body = toStringForHash();
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(body.data()), body.size(), digest);
    std::vector<uint8_t> v(digest, digest + SHA256_DIGEST_LENGTH);
    hash = utils::to_hex(v);
}

}  // namespace blockchain
