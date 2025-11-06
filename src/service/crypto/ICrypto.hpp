#pragma once
#include <string>
#include <vector>

namespace crypto
{

using Bytes = std::vector<uint8_t>;

struct KeyPair
{
    Bytes publicKey;
    Bytes privateKey;
};

class ICrypto
{
public:
    virtual ~ICrypto() = default;

    virtual Bytes generateSecret(size_t bytes = 32) = 0;
    virtual KeyPair generateKeyPair() = 0;

    virtual std::string keyToString(const Bytes &k) = 0;
    virtual Bytes stringToKey(const std::string &s) = 0;

    virtual Bytes createSessionKey(const Bytes &privateKey, const Bytes &peerPublicKey) = 0;

    virtual Bytes encrypt(const Bytes &message, const Bytes &key) = 0;
    virtual Bytes decrypt(const Bytes &cipher, const Bytes &key) = 0;

    virtual Bytes sign(const Bytes &message, const Bytes &privateKey) = 0;
    virtual bool verify(const Bytes &message, const Bytes &signature, const Bytes &publicKey) = 0;
};

}  // namespace crypto