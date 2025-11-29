#include "OpenSSLCrypto.hpp"

#include <openssl/buffer.h>
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
// return PEM-encoded key from OpenSSL EVP_PKEY format (standard struct for key storing)
// PEM-string like is a string like
// -----BEGIN EC PRIVATE KEY-----
// MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
// -----END EC PRIVATE KEY-----
std::string OpenSSLCrypto::pemFromEVP(EVP_PKEY* pkey, bool pub) noexcept
{
    if (!pkey) return {};
    BIO* bio = BIO_new(BIO_s_mem());  // create OpenSSL IO memory buffer
    if (!bio) return {};

    int ok = 0;
    if (pub)  // if public
    {
        ok = PEM_write_bio_PUBKEY(bio, pkey);  // write public key into buffer
    }
    else
    {
        // write private key without passphrase and cypher
        ok = PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr, nullptr);
    }
    if (!ok)
    {
        BIO_free(bio);  // free buffer in case of error
        return {};
    }

    BUF_MEM* bptr = nullptr;
    BIO_get_mem_ptr(bio, &bptr);  // get pointer to buffer in memory
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

// parse PEM string to OpenSSL EVP_PKEY, try private first, then public
EVP_PKEY* OpenSSLCrypto::evpFromPem(const std::string& pem) noexcept
{
    if (pem.empty()) return nullptr;

    // create OpenSSL buffer from PEM string
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) return nullptr;

    // first try to read private key
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    if (pkey)
    {
        BIO_free(bio);
        return pkey;
    }

    BIO_free(bio);  // free old buffer in case of error
    bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    if (!bio) return nullptr;

    pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);  // try to read public key
    BIO_free(bio);
    return pkey;  // may be nullptr
}

// encrypt message with AES-256-GCM
// Advanced Encryption Standard (AES) is a symmetric block cipher
// symmetric means that the same key uses for encryption and decryption
// AES-256-GCM - creates 256-bit AES key for encryption and 128-bit GCM tag for authentication
Bytes OpenSSLCrypto::aes_gcm_encrypt(const Bytes& key, const Bytes& plain)
{
    const size_t IV_LEN = 12;   // initialization vector (IV) length - unique for each encryption
    const size_t TAG_LEN = 16;  // tag length - confirms data integrity
    Bytes iv = generateSecret(IV_LEN);
    Bytes tag(TAG_LEN);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();  // create encryption OpenSSL context
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    // initialize encryption context with AES-256-GCM algorithm
    int rc = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    if (rc != 1)
    {
        EVP_CIPHER_CTX_free(ctx);  // free context in case of error
        throw std::runtime_error("EncryptInit failed");
    }

    // set length of initialization vector
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(IV_LEN), nullptr))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("set_ivlen failed");
    }
    // set key and initialization vector
    if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("set_key_iv failed");
    }

    Bytes cipher(plain.size());
    int outlen = 0;
    // encrypt data
    if (1 != EVP_EncryptUpdate(
                 ctx, cipher.data(), &outlen, plain.data(), static_cast<int>(plain.size())))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EncryptUpdate failed");
    }
    int cipher_len = outlen;
    if (1 != EVP_EncryptFinal_ex(ctx, cipher.data() + outlen, &outlen))  // finalize encryption
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("EncryptFinal failed");
    }
    cipher_len += outlen;
    cipher.resize(cipher_len);

    // get authentication tag, it computes automatically while encrypting
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, static_cast<int>(TAG_LEN), tag.data()))
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("GetTag failed");
    }

    EVP_CIPHER_CTX_free(ctx);

    Bytes out;
    out.reserve(iv.size() + tag.size() + cipher.size());
    out.insert(out.end(), iv.begin(), iv.end());          // IV
    out.insert(out.end(), tag.begin(), tag.end());        // tag
    out.insert(out.end(), cipher.begin(), cipher.end());  // cypher-text

    return out;  // IV(12)|tag(16)|cipher
}

// decrypt message with AES-256-GCM
Bytes OpenSSLCrypto::aes_gcm_decrypt(const Bytes& key, const Bytes& in)
{
    const size_t IV_LEN = 12, TAG_LEN = 16;
    if (in.size() < IV_LEN + TAG_LEN) throw std::runtime_error("cipher too short");

    // parse IV, tag and cipher-text
    Bytes iv(in.begin(), in.begin() + IV_LEN);
    Bytes tag(in.begin() + IV_LEN, in.begin() + IV_LEN + TAG_LEN);
    Bytes cipher(in.begin() + IV_LEN + TAG_LEN, in.end());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();  // create decryption OpenSSL context
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    if (1 != EVP_DecryptInit_ex(
                 ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr))  // init decryption context
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptInit failed");
    }
    if (1 !=
        EVP_CIPHER_CTX_ctrl(
            ctx, EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(iv.size()), nullptr))  // set IV length
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("set_ivlen failed");
    }
    if (1 != EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()))
    {  // set key and IV
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("set_key_iv failed");
    }

    Bytes out(cipher.size());
    int outlen = 0;
    if (1 != EVP_DecryptUpdate(ctx,
                               out.data(),
                               &outlen,
                               cipher.data(),
                               static_cast<int>(cipher.size())))  // data decryption
    {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptUpdate failed");
    }
    int total = outlen;
    if (1 != EVP_CIPHER_CTX_ctrl(
                 ctx, EVP_CTRL_GCM_SET_TAG, static_cast<int>(tag.size()), (void*)tag.data()))
    {  // set tag
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("SetTag failed");
    }
    if (1 != EVP_DecryptFinal_ex(ctx, out.data() + outlen, &outlen))
    {  // finalize decryption and tag checking (authentication)
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("DecryptFinal - tag mismatch");
    }

    total += outlen;
    out.resize(total);  // resize output to actual size
    EVP_CIPHER_CTX_free(ctx);

    return out;
}

OpenSSLCrypto::OpenSSLCrypto()
{
    // default initialization of OpenSSL
    OPENSSL_init_crypto(0, nullptr);
}

// cryptographically secure pseudorandom generator
// depends on system entropy
Bytes OpenSSLCrypto::generateSecret(size_t bytes)
{
    Bytes out(bytes);
    if (bytes == 0) return out;

    int rc = RAND_bytes(out.data(), static_cast<int>(bytes));  // write random bytes into out
    if (rc != 1) throw std::runtime_error("RAND_bytes failed");

    return out;
}

// convert binary key to base64
// base64 - data view format to converting binary to string
std::string OpenSSLCrypto::keyToString(const Bytes& k)
{
    if (k.empty()) return {};

    BIO* bio = BIO_new(BIO_s_mem());     // create OpenSSL memory buffer
    BIO* b64 = BIO_new(BIO_f_base64());  // create BIO-filter for base64

    bio = BIO_push(b64, bio);  // add b64 filter to buffer

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);            // set `without newlines` flag
    BIO_write(bio, k.data(), static_cast<int>(k.size()));  // filter and write binary data
    BIO_flush(bio);                                        // commit writing (already base64)

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);  // get pointer to decoded base64 buffer

    std::string encoded(bufferPtr->data, bufferPtr->length);  // copy cleared buffer
    BIO_free_all(bio);                                        // free buffer

    return encoded;
}

// convert base64 string to binary
Bytes OpenSSLCrypto::stringToKey(const std::string& s)
{
    if (s.empty()) return {};

    BIO* bio =
        BIO_new_mem_buf(s.data(), static_cast<int>(s.size()));  // create memory buffer from string
    BIO* b64 = BIO_new(BIO_f_base64());                         // create BIO-filter for base64

    bio = BIO_push(b64, bio);  // add b64 filter to buffer

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);  // set `without newlines` flag

    Bytes decoded(s.size());  // reserve memory
    int len = BIO_read(
        bio, decoded.data(), static_cast<int>(decoded.size()));  // read base64 and filter to binary
    BIO_free_all(bio);

    if (len < 0) len = 0;

    decoded.resize(len);  // resize to actual size (cutting off the excess)
    return decoded;
}

// generate elliptic curve (EC) keypair (P-256) - standard curve
// pair includes public and private key (asymmetric cryptography)
KeyPair OpenSSLCrypto::generateKeyPair()
{
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(
        EVP_PKEY_EC, nullptr);  // create key generation context with EC key type
    if (!ctx) throw std::runtime_error("EVP_PKEY_CTX_new_id failed");

    if (EVP_PKEY_keygen_init(ctx) <= 0)  // initialize key generation context
    {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("EVP_PKEY_keygen_init failed");
    }

    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) <= 0)  // set P-256 curve
    {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("EVP_PKEY_CTX_set_ec_paramgen_curve_nid failed");
    }

    EVP_PKEY* pkey = nullptr;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0)  // generate key pair
    {
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("EVP_PKEY_keygen failed");
    }

    EVP_PKEY_CTX_free(ctx);  // free key generation context

    // convert key pair to PEM-string from OpenSSL EVP_PKEY format
    // pkey
    KeyPair kp;
    std::string privPem =
        pemFromEVP(pkey, false);  // get private PEM-string key from pkey (d - big random number)
    std::string pubPem =
        pemFromEVP(pkey, true);  // select public key from pkey (d * G, G - base point of curve, it
                                 // is a mathematical representation of public key)

    // convert PEM-string to vector of bytes
    kp.publicKey = std::vector<unsigned char>(pubPem.begin(), pubPem.end());
    kp.privateKey = std::vector<unsigned char>(privPem.begin(), privPem.end());

    EVP_PKEY_free(pkey);
    return kp;
}

// create common session key (for A <-> B connection)
// Diffie-Hellman (DH) - a method that allows 2 peers to create shared secret key over an unsecured
// channel
// Elliptic Curve Diffie-Hellman (ECDH) - an extension of DH that uses elliptic curve cryptography
// HMAC-based Key Derivation Function (HKDF) - a key derivation function (функция формирования
// ключа), generate ECDH shared key and make it stable
Bytes OpenSSLCrypto::createSessionKey(const Bytes& privateKeyPem, const Bytes& peerPublicPem)
{
    // parse keys to EVP_PKEY objects
    EVP_PKEY* minePriv = evpFromPem(std::string(privateKeyPem.begin(), privateKeyPem.end()));
    EVP_PKEY* peerPublic = evpFromPem(std::string(peerPublicPem.begin(), peerPublicPem.end()));
    if (!minePriv || !peerPublic)
    {
        if (minePriv) EVP_PKEY_free(minePriv);
        if (peerPublic) EVP_PKEY_free(peerPublic);
        throw std::runtime_error("Invalid PEM keys");
    }

    // 1. ECDH - calculate common secret

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(minePriv, nullptr);  // create key derivation context
    // EVP_PKEY includes information about key generation algorithm (EC P-256)
    if (!ctx)
    {
        EVP_PKEY_free(minePriv);
        EVP_PKEY_free(peerPublic);
        throw std::runtime_error("EVP_PKEY_CTX_new failed");
    }

    if (1 != EVP_PKEY_derive_init(ctx))  // initialize key derivation
    {
        EVP_PKEY_free(minePriv);
        EVP_PKEY_free(peerPublic);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("derive_init failed");
    }
    if (1 != EVP_PKEY_derive_set_peer(ctx, peerPublic))  // set interlocutor's public key
    {
        EVP_PKEY_free(minePriv);
        EVP_PKEY_free(peerPublic);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("derive_set_peer failed");
    }

    size_t secret_len = 0;
    if (1 != EVP_PKEY_derive(ctx, nullptr, &secret_len))  // get secret length
    {
        EVP_PKEY_free(minePriv);
        EVP_PKEY_free(peerPublic);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("derive_len failed");
    }
    std::vector<uint8_t> secret(secret_len);
    if (1 != EVP_PKEY_derive(ctx, secret.data(), &secret_len))  // get secret (ECDH shared secret)
    {
        EVP_PKEY_free(minePriv);
        EVP_PKEY_free(peerPublic);
        EVP_PKEY_CTX_free(ctx);
        throw std::runtime_error("derive failed");
    }

    EVP_PKEY_free(minePriv);
    EVP_PKEY_free(peerPublic);
    EVP_PKEY_CTX_free(ctx);

    // 2. HKDF - improve ECDH shared secret and make it stable

    Bytes out(32);  // 32 bytes-long output

    EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);  // create HKDF context
    if (!kctx) throw std::runtime_error("HKDF ctx failed");
    if (1 != EVP_PKEY_derive_init(kctx))  // initialize context
    {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("HKDF init failed");
    }
    if (1 != EVP_PKEY_CTX_set_hkdf_md(kctx, EVP_sha256()))  // set hash-function for HKDF: SHA-256
    {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("HKDF set md failed");
    }
    if (1 != EVP_PKEY_CTX_set1_hkdf_key(
                 kctx,
                 secret.data(),
                 static_cast<int>(secret.size())))  // set ECDH shared secret as starting key
    {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("HKDF set key failed");
    }
    size_t olen = out.size();
    if (1 != EVP_PKEY_derive(kctx, out.data(), &olen))  // derive (compute) final key
    {
        EVP_PKEY_CTX_free(kctx);
        throw std::runtime_error("HKDF derive failed");
    }
    EVP_PKEY_CTX_free(kctx);

    return out;
}

Bytes OpenSSLCrypto::encrypt(const Bytes& message, const Bytes& key)
{
    if (key.size() < 32) throw std::runtime_error("Key too short for AES-256-GCM");
    return aes_gcm_encrypt(key, message);  // encrypt with AES-256-GCM
}

Bytes OpenSSLCrypto::decrypt(const Bytes& cipher, const Bytes& key)
{
    if (key.size() < 32) throw std::runtime_error("Key too short for AES-256-GCM");
    return aes_gcm_decrypt(key, cipher);  // decrypt with AES-256-GCM
}

// create cryptographic signature, that contains information about sender and confirms data
// integrity
// Elliptic Curve Digital Signature Algorithm (ECDSA) - algorithm for digital signatures on elliptic
// curves
Bytes OpenSSLCrypto::sign(const Bytes& message, const Bytes& privateKey)
{
    EVP_PKEY* pkey = evpFromPem(std::string(
        privateKey.begin(), privateKey.end()));  // convert private key PEM-string to EVP_PKEY
    if (!pkey) throw std::runtime_error("invalid private key PEM");

    EVP_MD_CTX* md = EVP_MD_CTX_new();  // create message digest context for sign computing
    if (!md)
    {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("EVP_MD_CTX_new failed");
    }
    // initialize MD context with SHA-256 hashing algorithm
    if (1 != EVP_DigestSignInit(md, nullptr, EVP_sha256(), nullptr, pkey))
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("DigestSignInit failed");
    }
    if (1 != EVP_DigestSignUpdate(
                 md, message.data(), message.size()))  // add data to sign and compute hash
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("DigestSignUpdate failed");
    }

    size_t sig_len = 0;
    if (1 != EVP_DigestSignFinal(md, nullptr, &sig_len))  // get signature length
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("DigestSignFinal(len) failed");
    }

    Bytes sig(sig_len);                                      // final signature buffer
    if (1 != EVP_DigestSignFinal(md, sig.data(), &sig_len))  // get signature
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("DigestSignFinal failed");
    }
    sig.resize(sig_len);  // resize signature

    // free resources
    EVP_MD_CTX_free(md);
    EVP_PKEY_free(pkey);

    return sig;
}

// verify cryptographic signature
// check that the signature was created with a private key corresponding to this public key
bool OpenSSLCrypto::verify(const Bytes& message, const Bytes& signature, const Bytes& publicKey)
{
    EVP_PKEY* pkey = evpFromPem(
        std::string(publicKey.begin(), publicKey.end()));  // parse public key to EVP_PKEY struct
    if (!pkey) return false;

    EVP_MD_CTX* md = EVP_MD_CTX_new();  // create message digest context for signature verification
    if (!md)
    {
        EVP_PKEY_free(pkey);
        return false;
    }
    // initialize MD for verifying, using SHA-256 (same as for signing)
    if (1 != EVP_DigestVerifyInit(md, nullptr, EVP_sha256(), nullptr, pkey))
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        return false;
    }
    if (1 != EVP_DigestVerifyUpdate(md, message.data(), message.size()))  // add message to verify
    {
        EVP_MD_CTX_free(md);
        EVP_PKEY_free(pkey);
        return false;
    }
    int rc = EVP_DigestVerifyFinal(md, signature.data(), signature.size());  // verify signature
    // rc = 1 - signature is valid
    // rc = 0 - signature is invalid
    // rc < 0 - error

    EVP_MD_CTX_free(md);
    EVP_PKEY_free(pkey);

    return rc == 1;
}
}  // namespace crypto