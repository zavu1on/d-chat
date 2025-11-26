#include <gtest/gtest.h>

#include "XORCrypto.hpp"

class XORCryptoTest : public ::testing::Test
{
protected:
    std::shared_ptr<crypto::ICrypto> crypto;

    void SetUp() override { crypto = std::make_shared<crypto::XORCrypto>(); }
};

TEST_F(XORCryptoTest, GenerateSecretProducesCorrectLength)
{
    auto secret32 = crypto->generateSecret(32);
    auto secret64 = crypto->generateSecret(64);

    EXPECT_EQ(secret32.size(), 32);
    EXPECT_EQ(secret64.size(), 64);
}

TEST_F(XORCryptoTest, GeneratedSecretsAreDifferent)
{
    auto secret1 = crypto->generateSecret(32);
    auto secret2 = crypto->generateSecret(32);

    EXPECT_NE(secret1, secret2);
}

TEST_F(XORCryptoTest, GenerateKeyPairProducesValidKeys)
{
    auto keyPair = crypto->generateKeyPair();

    EXPECT_FALSE(keyPair.publicKey.empty());
    EXPECT_FALSE(keyPair.privateKey.empty());
    EXPECT_EQ(keyPair.publicKey.size(), 32);
    EXPECT_EQ(keyPair.privateKey.size(), 32);
}

TEST_F(XORCryptoTest, KeyToStringAndBackIsReversible)
{
    auto original = crypto->generateSecret(32);
    auto str = crypto->keyToString(original);
    auto recovered = crypto->stringToKey(str);

    EXPECT_EQ(original, recovered);
    EXPECT_EQ(str.size(), 64);
}

TEST_F(XORCryptoTest, CreateSessionKeyProducesConsistentResult)
{
    auto keyPair1 = crypto->generateKeyPair();
    auto keyPair2 = crypto->generateKeyPair();

    auto sessionKey = crypto->createSessionKey(keyPair1.privateKey, keyPair2.publicKey);

    EXPECT_EQ(sessionKey.size(), 32);
}

TEST_F(XORCryptoTest, EncryptDecryptRoundTrip)
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

TEST_F(XORCryptoTest, EncryptWithDifferentKeysProducesDifferentCiphertext)
{
    std::string plaintext = "Test message";
    crypto::Bytes message(plaintext.begin(), plaintext.end());

    auto key1 = crypto->generateSecret(32);
    auto key2 = crypto->generateSecret(32);

    auto encrypted1 = crypto->encrypt(message, key1);
    auto encrypted2 = crypto->encrypt(message, key2);

    EXPECT_NE(encrypted1, encrypted2);
}

TEST_F(XORCryptoTest, DecryptWithWrongKeyFails)
{
    std::string plaintext = "Secret message";
    crypto::Bytes message(plaintext.begin(), plaintext.end());

    auto key1 = crypto->generateSecret(32);
    auto key2 = crypto->generateSecret(32);

    auto encrypted = crypto->encrypt(message, key1);
    auto decrypted = crypto->decrypt(encrypted, key2);

    EXPECT_NE(decrypted, message);
}

TEST_F(XORCryptoTest, EncryptPreservesMessageLength)
{
    std::string plaintext = "Variable length message!";
    crypto::Bytes message(plaintext.begin(), plaintext.end());
    crypto::Bytes key = crypto->generateSecret(32);

    auto encrypted = crypto->encrypt(message, key);

    EXPECT_EQ(encrypted.size(), message.size());
}

TEST_F(XORCryptoTest, SignProducesConsistentSignature)
{
    std::string text = "Message to sign";
    crypto::Bytes message(text.begin(), text.end());
    auto keyPair = crypto->generateKeyPair();

    auto signature1 = crypto->sign(message, keyPair.privateKey);
    auto signature2 = crypto->sign(message, keyPair.privateKey);

    EXPECT_EQ(signature1.size(), 32);
    EXPECT_EQ(signature1, signature2);
}

TEST_F(XORCryptoTest, DifferentMessagesProduceDifferentSignatures)
{
    crypto::Bytes message1 = { 'H', 'e', 'l', 'l', 'o' };
    crypto::Bytes message2 = { 'W', 'o', 'r', 'l', 'd' };
    auto keyPair = crypto->generateKeyPair();

    auto signature1 = crypto->sign(message1, keyPair.privateKey);
    auto signature2 = crypto->sign(message2, keyPair.privateKey);

    EXPECT_NE(signature1, signature2);
}

TEST_F(XORCryptoTest, VerifyAcceptsValidSignature)
{
    std::string text = "Message";
    crypto::Bytes message(text.begin(), text.end());
    auto keyPair = crypto->generateKeyPair();

    auto signature = crypto->sign(message, keyPair.privateKey);
    bool result = crypto->verify(message, signature, keyPair.publicKey);

    EXPECT_TRUE(result);
}

TEST_F(XORCryptoTest, VerifyRejectsWrongLengthSignature)
{
    std::string text = "Message";
    crypto::Bytes message(text.begin(), text.end());
    auto keyPair = crypto->generateKeyPair();

    crypto::Bytes shortSignature(16, 0x42);
    crypto::Bytes longSignature(64, 0x42);

    EXPECT_FALSE(crypto->verify(message, shortSignature, keyPair.publicKey));
    EXPECT_FALSE(crypto->verify(message, longSignature, keyPair.publicKey));
}

TEST_F(XORCryptoTest, VerifyRejectsFakeSignature)
{
    std::string text = "Important message";
    crypto::Bytes message(text.begin(), text.end());
    auto keyPair = crypto->generateKeyPair();

    crypto::Bytes fakeSignature(32, 0x00);

    bool result = crypto->verify(message, fakeSignature, keyPair.publicKey);

    EXPECT_FALSE(result);
}
