#include <openssl/sha.h>

#include <string>
#include <vector>

#include "hex.hpp"

namespace utils
{
inline std::string sha256(const std::string& data)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest);
    std::vector<uint8_t> v(digest, digest + SHA256_DIGEST_LENGTH);
    return utils::toHex(v);
}
}  // namespace utils