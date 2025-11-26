/**
 * @file xor_crypto_test.cpp
 * @brief Unit-тесты для класса XORCrypto
 *
 * XORCrypto — упрощённая реализация криптографии для демонстрационных целей.
 * НЕ является криптографически безопасной.
 */

#include <gtest/gtest.h>

#include "XORCrypto.hpp"

class XORCryptoTest : public ::testing::Test
{
protected:
    std::shared_ptr<crypto::ICrypto> crypto;

    void SetUp() override { crypto = std::make_shared<crypto::XORCrypto>(); }
};

TEST_F(XORCryptoTest, FullWorkflowKeyExchangeAndEncryption)
{
    auto aliceKeys = crypto->generateKeyPair();
    auto bobKeys = crypto->generateKeyPair();

    auto aliceSessionKey = crypto->createSessionKey(aliceKeys.privateKey, bobKeys.publicKey);
    auto bobSessionKey = crypto->createSessionKey(bobKeys.privateKey, aliceKeys.publicKey);

    std::string plaintext = "Hello Bob!";
    crypto::Bytes message(plaintext.begin(), plaintext.end());
    auto encrypted = crypto->encrypt(message, aliceSessionKey);

    auto decrypted = crypto->decrypt(encrypted, bobSessionKey);

    std::string recoveredText(decrypted.begin(), decrypted.end());
    EXPECT_EQ(recoveredText, plaintext);
}