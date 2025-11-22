#include "Block.hpp"

#include <openssl/sha.h>

#include <sstream>

#include "sha256.hpp"

namespace blockchain
{
Block::Block()
{
    hash = "0";
    previousHash = "0";
    payloadHash = "0";
    authorPublicKey = "0";
    signature = "0";
    timestamp = 0;
}

Block::Block(std::string hash,
             std::string previousHash,
             std::string payloadHash,
             std::string authorPublicKey,
             std::string signature,
             uint64_t timestamp)
    : hash(hash),
      previousHash(previousHash),
      payloadHash(payloadHash),
      authorPublicKey(authorPublicKey),
      signature(signature),
      timestamp(timestamp)
{
}

Block::Block(const json& jData)
{
    hash = jData["hash"].get<std::string>();
    previousHash = jData["previousHash"].get<std::string>();
    payloadHash = jData["payloadHash"].get<std::string>();
    authorPublicKey = jData["authorPubKey"].get<std::string>();
    signature = jData["signature"].get<std::string>();
    timestamp = jData["timestamp"].get<uint64_t>();
}

std::string Block::toStringForHash() const
{
    std::ostringstream oss;
    oss << previousHash << "|" << payloadHash << "|" << authorPublicKey << "|" << timestamp;
    return oss.str();
}

void Block::computeHash()
{
    std::string body = toStringForHash();
    hash = utils::sha256(body);
}

json Block::toJson() const
{
    json jData;
    jData["previousHash"] = previousHash;
    jData["payloadHash"] = payloadHash;
    jData["authorPubKey"] = authorPublicKey;
    jData["signature"] = signature;
    jData["timestamp"] = timestamp;
    jData["hash"] = hash;
    return jData;
}
}  // namespace blockchain