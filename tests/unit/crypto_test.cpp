#include <gtest/gtest.h>

#include "OpenSSLCrypto.hpp"
#include "XORCrypto.hpp"

class CryptoTest : public ::testing::Test
{
protected:
    std::shared_ptr<crypto::ICrypto> crypto;

    void SetUp() override { crypto = std::make_shared<crypto::OpenSSLCrypto>(); }
};

TEST_F(CryptoTest, GenerateSecretProducesCorrectLength)
{
    auto secret = crypto->generateSecret(32);
    EXPECT_EQ(secret.size(), 32);

    auto secret64 = crypto->generateSecret(64);
    EXPECT_EQ(secret64.size(), 64);
}

TEST_F(CryptoTest, GeneratedSecretsAreDifferent)
{
    auto secret1 = crypto->generateSecret(32);
    auto secret2 = crypto->generateSecret(32);
    EXPECT_NE(secret1, secret2);
}

TEST_F(CryptoTest, GenerateKeyPairProducesValidKeys)
{
    auto keyPair = crypto->generateKeyPair();

    EXPECT_FALSE(keyPair.publicKey.empty());
    EXPECT_FALSE(keyPair.privateKey.empty());
    EXPECT_NE(keyPair.publicKey, keyPair.privateKey);
}

TEST_F(CryptoTest, KeyToStringAndBackIsReversible)
{
    auto original = crypto->generateSecret(32);
    auto str = crypto->keyToString(original);
    auto recovered = crypto->stringToKey(str);

    EXPECT_EQ(original, recovered);
}

TEST_F(CryptoTest, EncryptDecryptRoundTrip)
{
    std::string plaintext = "Hello, World! This is a test message.";
    crypto::Bytes message(plaintext.begin(), plaintext.end());
    crypto::Bytes key = crypto->generateSecret(32);

    auto encrypted = crypto->encrypt(message, key);
    EXPECT_NE(encrypted, message);

    auto decrypted = crypto->decrypt(encrypted, key);
    EXPECT_EQ(decrypted, message);

    std::string recoveredText(decrypted.begin(), decrypted.end());
    EXPECT_EQ(recoveredText, plaintext);
}

TEST_F(CryptoTest, EncryptWithDifferentKeysProducesDifferentCiphertext)
{
    std::string plaintext = "Test message";
    crypto::Bytes message(plaintext.begin(), plaintext.end());

    auto key1 = crypto->generateSecret(32);
    auto key2 = crypto->generateSecret(32);

    auto encrypted1 = crypto->encrypt(message, key1);
    auto encrypted2 = crypto->encrypt(message, key2);

    EXPECT_NE(encrypted1, encrypted2);
}

TEST_F(CryptoTest, DecryptWithWrongKeyFails)
{
    std::string plaintext = "Secret message";
    crypto::Bytes message(plaintext.begin(), plaintext.end());

    auto key1 = crypto->generateSecret(32);
    auto key2 = crypto->generateSecret(32);

    auto encrypted = crypto->encrypt(message, key1);

    EXPECT_THROW({ crypto->decrypt(encrypted, key2); }, std::runtime_error);
}

TEST_F(CryptoTest, SignAndVerifyWorks)
{
    auto keyPair = crypto->generateKeyPair();

    std::string data = "Data to be signed";
    crypto::Bytes message(data.begin(), data.end());

    auto signature = crypto->sign(message, keyPair.privateKey);
    EXPECT_FALSE(signature.empty());

    bool isValid = crypto->verify(message, signature, keyPair.publicKey);
    EXPECT_TRUE(isValid);
}

TEST_F(CryptoTest, VerifyFailsWithWrongPublicKey)
{
    auto keyPair1 = crypto->generateKeyPair();
    auto keyPair2 = crypto->generateKeyPair();

    std::string data = "Data to be signed";
    crypto::Bytes message(data.begin(), data.end());

    auto signature = crypto->sign(message, keyPair1.privateKey);

    bool isValid = crypto->verify(message, signature, keyPair2.publicKey);
    EXPECT_FALSE(isValid);
}

TEST_F(CryptoTest, VerifyFailsWithModifiedMessage)
{
    auto keyPair = crypto->generateKeyPair();

    std::string data = "Original message";
    crypto::Bytes message(data.begin(), data.end());

    auto signature = crypto->sign(message, keyPair.privateKey);

    std::string modifiedData = "Modified message";
    crypto::Bytes modifiedMessage(modifiedData.begin(), modifiedData.end());

    bool isValid = crypto->verify(modifiedMessage, signature, keyPair.publicKey);
    EXPECT_FALSE(isValid);
}

TEST_F(CryptoTest, CreateSessionKeyIsDeterministic)
{
    auto keyPair1 = crypto->generateKeyPair();
    auto keyPair2 = crypto->generateKeyPair();

    auto sessionKey1 = crypto->createSessionKey(keyPair1.privateKey, keyPair2.publicKey);
    auto sessionKey2 = crypto->createSessionKey(keyPair1.privateKey, keyPair2.publicKey);

    EXPECT_EQ(sessionKey1, sessionKey2);
}

TEST_F(CryptoTest, SessionKeyIsSymmetric)
{
    auto keyPair1 = crypto->generateKeyPair();
    auto keyPair2 = crypto->generateKeyPair();

    auto sessionKey_1_2 = crypto->createSessionKey(keyPair1.privateKey, keyPair2.publicKey);
    auto sessionKey_2_1 = crypto->createSessionKey(keyPair2.privateKey, keyPair1.publicKey);

    EXPECT_EQ(sessionKey_1_2, sessionKey_2_1);
}