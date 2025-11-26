
#include "XORCrypto.hpp"

#include <algorithm>
#include <chrono>

#include "hex.hpp"

namespace crypto
{

Bytes XORCrypto::sha256_bytes(const Bytes& data)
{
    Bytes out(32, 0);
    for (size_t i = 0; i < data.size(); ++i)
    {
        out[i % 32] ^= data[i];
    }
    return out;
}

XORCrypto::XORCrypto()
{
    m_rng.seed(static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
}

Bytes XORCrypto::generateSecret(size_t bytes)
{
    Bytes out(bytes);
    for (size_t i = 0; i < bytes; ++i)
    {
        out[i] = static_cast<uint8_t>(m_rng() & 0xFF);
    }
    return out;
}

KeyPair XORCrypto::generateKeyPair()
{
    KeyPair kp;
    Bytes secret = generateSecret(32);
    kp.privateKey = secret;
    kp.publicKey = secret;  // publicKey == privateKey
    return kp;
}

std::string XORCrypto::keyToString(const Bytes& k) { return utils::toHex(k); }

Bytes XORCrypto::stringToKey(const std::string& s) { return utils::fromHex(s); }

Bytes XORCrypto::createSessionKey(const Bytes& privateKey, const Bytes& peerPublicKey)
{
    size_t n = std::max(privateKey.size(), peerPublicKey.size());
    Bytes tmp(n);
    for (size_t i = 0; i < n; ++i)
    {
        tmp[i] = privateKey[i % privateKey.size()] ^ peerPublicKey[i % peerPublicKey.size()];
    }

    Bytes out(32, 0);
    for (size_t i = 0; i < tmp.size(); ++i)
    {
        out[i % 32] ^= tmp[i];
    }
    return out;
}

Bytes XORCrypto::encrypt(const Bytes& message, const Bytes& key)
{
    Bytes out(message.size());
    if (key.empty()) return out;

    for (size_t i = 0; i < message.size(); ++i)
    {
        out[i] = message[i] ^ key[i % key.size()];
    }
    return out;
}

Bytes XORCrypto::decrypt(const Bytes& cipher, const Bytes& key) { return encrypt(cipher, key); }

Bytes XORCrypto::sign(const Bytes& message, const Bytes& privateKey)
{
    Bytes concat;
    concat.reserve(privateKey.size() + message.size());
    concat.insert(concat.end(), privateKey.begin(), privateKey.end());
    concat.insert(concat.end(), message.begin(), message.end());
    return sha256_bytes(concat);
}

bool XORCrypto::verify(const Bytes& message, const Bytes& signature, const Bytes& publicKey)
{
    Bytes concat;
    concat.reserve(publicKey.size() + message.size());
    concat.insert(concat.end(), publicKey.begin(), publicKey.end());
    concat.insert(concat.end(), message.begin(), message.end());

    Bytes expectedSignature = sha256_bytes(concat);

    if (signature.size() != expectedSignature.size()) return false;

    uint8_t diff = 0;
    for (size_t i = 0; i < signature.size(); ++i)
    {
        diff |= signature[i] ^ expectedSignature[i];
    }

    return diff == 0;
}
}  // namespace crypto