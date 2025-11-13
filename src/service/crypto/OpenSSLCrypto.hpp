#pragma once
#include <openssl/evp.h>

#include "../utils/hex.hpp"
#include "ICrypto.hpp"

namespace crypto
{
class OpenSSLCrypto : public ICrypto
{
public:
    OpenSSLCrypto();
    ~OpenSSLCrypto() override;

    Bytes generateSecret(size_t bytes = 32) override;

    KeyPair generateKeyPair() override;  // returns PEM-encoded keys in Bytes
    std::string keyToString(const Bytes& k) override;
    Bytes stringToKey(const std::string& s) override;

    Bytes createSessionKey(const Bytes& privateKey, const Bytes& peerPublicKey) override;

    Bytes encrypt(const Bytes& message,
                  const Bytes& key) override;  // AES-256-GCM result = iv(12)|tag(16)|cipher
    Bytes decrypt(const Bytes& cipher, const Bytes& key) override;

    Bytes sign(const Bytes& message, const Bytes& privateKey) override;  // ECDSA
    bool verify(const Bytes& message, const Bytes& signature, const Bytes& publicKey) override;

private:
    void initOpenSSL() noexcept;
    void cleanupOpenSSL() noexcept;

    std::string pemFromEVP(EVP_PKEY* pkey, bool pub) noexcept;
    EVP_PKEY* evpFromPem(const std::string& pem) noexcept;

    Bytes aes_gcm_encrypt(const Bytes& key, const Bytes& plain);
    Bytes aes_gcm_decrypt(const Bytes& key, const Bytes& in);  // in = iv|tag|cipher

    // helper
    static void throwIf(bool cond, const char* msg);
};
}  // namespace crypto