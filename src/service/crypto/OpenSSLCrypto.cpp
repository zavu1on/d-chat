#include "OpenSSLCrypto.hpp"

#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace crypto
{
void OpenSSLCrypto::throwIf(bool cond, const char* msg)
{
    if (cond) throw std::runtime_error(msg);
}

OpenSSLCrypto::OpenSSLCrypto() { initOpenSSL(); }
OpenSSLCrypto::~OpenSSLCrypto() { cleanupOpenSSL(); }

void OpenSSLCrypto::initOpenSSL() noexcept { OPENSSL_init_crypto(0, nullptr); }

void OpenSSLCrypto::cleanupOpenSSL() noexcept
{
    // modern OpenSSL doesn't require explicit cleanup for our usage
}

// Returns cryptographic random bytes
Bytes OpenSSLCrypto::generateSecret(size_t bytes)
{
    Bytes out(bytes);
    if (bytes == 0) return out;
    int rc = RAND_bytes(out.data(), static_cast<int>(bytes));
    if (rc != 1) throw std::runtime_error("RAND_bytes failed");
    return out;
}

//
// Helper: serialize EVP_PKEY to PEM. Returns empty string on error.
// Always safe: checks PEM_write result and BIO_get_mem_data length.
//
std::string OpenSSLCrypto::pemFromEVP(EVP_PKEY* pkey, bool pub) noexcept
{
    if (!pkey) return {};
    BIO* bio = BIO_new(BIO_s_mem());
    if (!bio) return {};

    int ok = 0;
    if (pub)
    {
        ok = PEM_write_bio_PUBKEY(bio, pkey);
    }
    else
    {
        ok = PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    }
    if (!ok)
    {
        BIO_free(bio);
        return {};
    }

    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(bio, &bptr);
    if (!bptr || bptr->length <= 0)
    {
        BIO_free(bio);
        return {};
    }

    // copy data safely
    std::string s(bptr->data, static_cast<size_t>(bptr->length));
    BIO_free(bio);
    return s;
}

// Parse PEM: try private first, then public. Returns EVP_PKEY* or nullptr.
// Caller must free EVP_PKEY with EVP_PKEY_free.
EVP_PKEY* OpenSSLCrypto::evpFromPem(const std::string& pem) noexcept
{
    if (pem.empty()) return nullptr;
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) return nullptr;

    // First try private key
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    if (pkey)
    {
        BIO_free(bio);
        return pkey;
    }

    // try public
    BIO_free(bio);
    bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) return nullptr;
    pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    return pkey;  // may be nullptr
}

#include <openssl/buffer.h>
#include <openssl/evp.h>

std::string OpenSSLCrypto::keyToString(const Bytes& k)
{
    if (k.empty()) return {};

    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);  // без переносов строк
    BIO_write(bio, k.data(), static_cast<int>(k.size()));
    BIO_flush(bio);

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string encoded(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    return encoded;
}

Bytes OpenSSLCrypto::stringToKey(const std::string& s)
{
    if (s.empty()) return {};

    BIO* bio = BIO_new_mem_buf(s.data(), static_cast<int>(s.size()));
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);  // без переносов строк

    Bytes decoded(s.size());
    int len = BIO_read(bio, decoded.data(), static_cast<int>(decoded.size()));
    BIO_free_all(bio);

    if (len < 0) len = 0;
    decoded.resize(len);
    return decoded;
}

// Generate EC keypair (P-256), return PEM-encoded keys in bytes
KeyPair OpenSSLCrypto::generateKeyPair()
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    if (!ctx) throw std::runtime_error("EVP_PKEY_CTX_new_id failed");

    if (EVP_PKEY_keygen_init(ctx) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("EVP_PKEY_keygen_init failed");
    }

    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("EVP_PKEY_CTX_set_ec_paramgen_curve_nid failed");
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0)
    {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("EVP_PKEY_keygen failed");
    }

    EVP_PKEY_CTX_free(ctx);

    KeyPair kp;
    std::string pubPem = pemFromEVP(pkey, true);
    std::string privPem = pemFromEVP(pkey, false);

    kp.publicKey = std::vector<unsigned char>(pubPem.begin(), pubPem.end());
    kp.privateKey = std::vector<unsigned char>(privPem.begin(), privPem.end());

    EVP_PKEY_free(pkey);
    return kp;
}

// ECDH derive + HKDF-SHA256 (derive to 32 bytes)
Bytes OpenSSLCrypto::createSessionKey(const Bytes& privateKeyPem, const Bytes& peerPublicPem)
{
    // parse keys
    EVP_PKEY* priv = evpFromPem(std::string(privateKeyPem.begin(), privateKeyPem.end()));
    EVP_PKEY* peer = evpFromPem(std::string(peerPublicPem.begin(), peerPublicPem.end()));
    if (!priv || !peer)
    {
        if (priv) EVP_PKEY_free(priv);
        if (peer) EVP_PKEY_free(peer);
        throw std::runtime_error("Invalid PEM keys");
    }

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(priv, nullptr);
    if (!ctx)
    {
        EVP_PKEY_free(priv);
        EVP_PKEY_free(peer);
        throw std::runtime_error("EVP_PKEY_CTX_new failed");
    }

    if (1 != EVP_PKEY_derive_init(ctx))
    {
        EVP_PKEY_free(priv);
        EVP_PKEY_free(peer);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("derive_init failed");
    }
    if (1 != EVP_PKEY_derive_set_peer(ctx, peer))
    {
        EVP_PKEY_free(priv);
        EVP_PKEY_free(peer);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("derive_set_peer failed");
    }

    size_t secret_len = 0;
    if (1 != EVP_PKEY_derive(ctx, nullptr, &secret_len))
    {
        EVP_PKEY_free(priv);
        EVP_PKEY_free(peer);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("derive_len failed");
    }
    std::vector<uint8_t> secret(secret_len);
    if (1 != EVP_PKEY_derive(ctx, secret.data(), &secret_len))
    {
        EVP_PKEY_free(priv);
        EVP_PKEY_free(peer);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("derive failed");
    }

    EVP_PKEY_free(priv);
    EVP_PKEY_free(peer);
    EVP_PKEY_CTX_free(ctx);

    // HKDF-SHA256 to 32 bytes
    Bytes out(32);
    EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
    if (!kctx) throw std::runtime_error("HKDF ctx failed");
    if (1 != EVP_PKEY_derive_init(kctx))
    {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("HKDF init failed");
    }
    if (1 != EVP_PKEY_CTX_set_hkdf_md(kctx, EVP_sha256()))
    {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("HKDF set md failed");
    }
    // no salt & no info
    if (1 != EVP_PKEY_CTX_set1_hkdf_key(kctx, secret.data(), static_cast<int>(secret.size())))
    {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("HKDF set key failed");
    }
    size_t olen = out.size();
    if (1 != EVP_PKEY_derive(kctx, out.data(), &olen))
    {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("HKDF derive failed");
    }
    EVP_PKEY_CTX_free(kctx);

    return out;
}

// AES-256-GCM helpers
Bytes OpenSSLCrypto::aes_gcm_encrypt(const Bytes& key, const Bytes& plain)
{
    const size_t IV_LEN = 12;
    const size_t TAG_LEN = 16;
    Bytes iv = generateSecret(IV_LEN);
    Bytes tag(TAG_LEN);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    int rc = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    if (rc != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EncryptInit failed");
    }
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(IV_LEN), nullptr))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("set_ivlen failed");
    }
    if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("set_key_iv failed");
    }

    Bytes cipher(plain.size());
    int outlen = 0;
    if (1 != EVP_EncryptUpdate(
                 ctx, cipher.data(), &outlen, plain.data(), static_cast<int>(plain.size())))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EncryptUpdate failed");
    }
    int cipher_len = outlen;
    if (1 != EVP_EncryptFinal_ex(ctx, cipher.data() + outlen, &outlen))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EncryptFinal failed");
    }
    cipher_len += outlen;
    cipher.resize(cipher_len);

    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(TAG_LEN), tag.data()))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("GetTag failed");
    }

    EVP_CIPHER_CTX_free(ctx);

    Bytes out;
    out.reserve(iv.size() + tag.size() + cipher.size());
    out.insert(out.end(), iv.begin(), iv.end());
    out.insert(out.end(), tag.begin(), tag.end());
    out.insert(out.end(), cipher.begin(), cipher.end());
    return out;
}

Bytes OpenSSLCrypto::aes_gcm_decrypt(const Bytes& key, const Bytes& in)
{
    const size_t IV_LEN = 12, TAG_LEN = 16;
    if (in.size() < IV_LEN + TAG_LEN) throw std::runtime_error("cipher too short");
    Bytes iv(in.begin(), in.begin() + IV_LEN);
    Bytes tag(in.begin() + IV_LEN, in.begin() + IV_LEN + TAG_LEN);
    Bytes cipher(in.begin() + IV_LEN + TAG_LEN, in.end());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptInit failed");
    }
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("set_ivlen failed");
    }
    if (1 != EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("set_key_iv failed");
    }

    Bytes out(cipher.size());
    int outlen = 0;
    if (1 !=
        EVP_DecryptUpdate(ctx, out.data(), &outlen, cipher.data(), static_cast<int>(cipher.size())))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptUpdate failed");
    }
    int total = outlen;
    if (1 != EVP_CIPHER_CTX_ctrl(
                 ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), (void*)tag.data()))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("SetTag failed");
    }
    if (1 != EVP_DecryptFinal_ex(ctx, out.data() + outlen, &outlen))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptFinal - tag mismatch");
    }
    total += outlen;
    out.resize(total);
    EVP_CIPHER_CTX_free(ctx);
    return out;
}

Bytes OpenSSLCrypto::encrypt(const Bytes& message, const Bytes& key)
{
    if (key.size() < 32) throw std::runtime_error("Key too short for AES-256-GCM");
    return aes_gcm_encrypt(key, message);
}
Bytes OpenSSLCrypto::decrypt(const Bytes& cipher, const Bytes& key)
{
    if (key.size() < 32) throw std::runtime_error("Key too short for AES-256-GCM");
    return aes_gcm_decrypt(key, cipher);
}

// ECDSA sign (privateKey is PEM string bytes)
Bytes OpenSSLCrypto::sign(const Bytes& message, const Bytes& privateKey)
{
    EVP_PKEY* pkey = evpFromPem(std::string(privateKey.begin(), privateKey.end()));
    if (!pkey) throw std::runtime_error("invalid private key PEM");

    EVP_MD_CTX* md = EVP_MD_CTX_new();
    if (!md)
    {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }
    if (1 != EVP_DigestSignInit(md, nullptr, EVP_sha256(), nullptr, pkey))
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("DigestSignInit failed");
    }
    if (1 != EVP_DigestSignUpdate(md, message.data(), message.size()))
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("DigestSignUpdate failed");
    }
    size_t sig_len = 0;
    if (1 != EVP_DigestSignFinal(md, nullptr, &sig_len))
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("DigestSignFinal(len) failed");
    }
    Bytes sig(sig_len);
    if (1 != EVP_DigestSignFinal(md, sig.data(), &sig_len))
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("DigestSignFinal failed");
    }
    sig.resize(sig_len);
    EVP_MD_CTX_free(md);
    EVP_PKEY_free(pkey);
    return sig;
}

bool OpenSSLCrypto::verify(const Bytes& message, const Bytes& signature, const Bytes& publicKey)
{
    EVP_PKEY* pkey = evpFromPem(std::string(publicKey.begin(), publicKey.end()));
    if (!pkey) return false;
    EVP_MD_CTX* md = EVP_MD_CTX_new();
    if (!md)
    {
        EVP_PKEY_free(pkey);
        return false;
    }
    if (1 != EVP_DigestVerifyInit(md, nullptr, EVP_sha256(), nullptr, pkey))
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        return false;
    }
    if (1 != EVP_DigestVerifyUpdate(md, message.data(), message.size()))
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        return false;
    }
    int rc = EVP_DigestVerifyFinal(md, signature.data(), signature.size());
    EVP_MD_CTX_free(md);
    EVP_PKEY_free(pkey);
    return rc == 1;
}
}  // namespace crypto