#include "XORCrypto.hpp"

#include <algorithm>
#include <chrono>

#include "hex.hpp"

// for sha256 use minimal implementation via std::hash-like fallback if OpenSSL absent.
// For simplicity here we use a tiny wrapper to std::hash on blocks (not cryptographically secure)
// but for integrity (block hashing) we'll use OpenSSLCrypto if available.
// NOTE: This XORCrypto is for demo; you already have OpenSSL implementation for production.

namespace crypto
{

Bytes XORCrypto::sha256_bytes(const Bytes& data)
{
    // fallback tiny hash: not secure; but used only in XORCrypto sign placeholder.
    // Use OpenSSLCrypto for production hashing.
    Bytes out(32, 0);
    for (size_t i = 0; i < data.size(); ++i) out[i % 32] ^= data[i];
    return out;
}

XORCrypto::XORCrypto()
{
    m_rng.seed(static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
}

Bytes XORCrypto::generateSecret(size_t bytes)
{
    Bytes out(bytes);
    for (size_t i = 0; i < bytes; ++i) out[i] = static_cast<uint8_t>(m_rng() & 0xFF);
    return out;
}

KeyPair XORCrypto::generateKeyPair()
{
    KeyPair kp;
    kp.privateKey = generateSecret(32);
    kp.publicKey = generateSecret(32);
    return kp;
}

std::string XORCrypto::keyToString(const Bytes& k) { return utils::to_hex(k); }
Bytes XORCrypto::stringToKey(const std::string& s) { return utils::from_hex(s); }

Bytes XORCrypto::createSessionKey(const Bytes& privateKey, const Bytes& peerPublicKey)
{
    size_t n = std::max(privateKey.size(), peerPublicKey.size());
    Bytes tmp(n);
    for (size_t i = 0; i < n; ++i)
    {
        tmp[i] = privateKey[i % privateKey.size()] ^ peerPublicKey[i % peerPublicKey.size()];
    }
    // reduce to 32 bytes by simple folding/XOR (not cryptographically ideal, but consistent)
    Bytes out(32, 0);
    for (size_t i = 0; i < tmp.size(); ++i) out[i % 32] ^= tmp[i];
    return out;
}

Bytes XORCrypto::encrypt(const Bytes& message, const Bytes& key)
{
    Bytes out(message.size());
    if (key.empty()) return out;
    for (size_t i = 0; i < message.size(); ++i) out[i] = message[i] ^ key[i % key.size()];
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
    (void)message;
    (void)publicKey;
    return signature.size() == 32;
}

}  // namespace crypto
