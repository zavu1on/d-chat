#pragma once
#include <random>

#include "ICrypto.hpp"

namespace crypto
{
class XORCrypto : public ICrypto
{
private:
    std::mt19937_64 m_rng;
    // small internal helper (simple wrapper to a portable SHA-256 impl if available)
    Bytes sha256_bytes(const Bytes& data);

public:
    XORCrypto();
    ~XORCrypto() override = default;

    Bytes generateSecret(size_t bytes = 32) override;
    KeyPair generateKeyPair() override;

    std::string keyToString(const Bytes& k) override;
    Bytes stringToKey(const std::string& s) override;

    Bytes createSessionKey(const Bytes& privateKey, const Bytes& peerPublicKey) override;

    Bytes encrypt(const Bytes& message, const Bytes& key) override;
    Bytes decrypt(const Bytes& cipher, const Bytes& key) override;

    Bytes sign(const Bytes& message, const Bytes& privateKey) override;
    bool verify(const Bytes& message, const Bytes& signature, const Bytes& publicKey) override;
};

}  // namespace crypto
