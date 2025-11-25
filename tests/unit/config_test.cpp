#include <gtest/gtest.h>

#include <fstream>

#include "JsonConfig.hpp"
#include "OpenSSLCrypto.hpp"
#include "test_helpers.hpp"

class ConfigTest : public ::testing::Test
{
protected:
    std::shared_ptr<crypto::ICrypto> crypto;
    std::unique_ptr<test_helpers::TestEnvironment> env;

    void SetUp() override
    {
        crypto = std::make_shared<crypto::OpenSSLCrypto>();
        env = std::make_unique<test_helpers::TestEnvironment>();
    }

    void TearDown() override { env->cleanup(); }
};

TEST_F(ConfigTest, LoadValidConfigSucceeds)
{
    std::string configPath = env->createTestConfig(8001, "priv123", "pub456");

    auto config = std::make_shared<config::JsonConfig>(configPath, crypto);

    EXPECT_TRUE(config->isValid());
    EXPECT_EQ(config->get(config::ConfigField::HOST), "127.0.0.1");
    EXPECT_EQ(config->get(config::ConfigField::PORT), "8001");
    EXPECT_EQ(config->get(config::ConfigField::PRIVATE_KEY), "priv123");
    EXPECT_EQ(config->get(config::ConfigField::PUBLIC_KEY), "pub456");
}

TEST_F(ConfigTest, GenerateDefaultConfigWhenMissing)
{
    std::string configPath = env->createTempFile("missing_config.json");

    auto config = std::make_shared<config::JsonConfig>(configPath, crypto);

    EXPECT_TRUE(config->isValid());

    config->generatedDefaultConfig();

    EXPECT_TRUE(test_helpers::fileExists(configPath));

    // Reload and verify
    auto reloadedConfig = std::make_shared<config::JsonConfig>(configPath, crypto);
    EXPECT_TRUE(reloadedConfig->isValid());
    EXPECT_FALSE(reloadedConfig->get(config::ConfigField::HOST).empty());
    EXPECT_FALSE(reloadedConfig->get(config::ConfigField::PORT).empty());
}

TEST_F(ConfigTest, LoadTrustedPeerListSucceeds)
{
    std::string configPath = env->createTempFile("config_with_peers.json");

    nlohmann::json jData;
    jData["host"] = "127.0.0.1";
    jData["port"] = "8001";
    jData["private_key"] = "priv";
    jData["public_key"] = "pub";
    jData["trustedPeerList"] = nlohmann::json::array();
    jData["trustedPeerList"].push_back("127.0.0.1:8002");
    jData["trustedPeerList"].push_back("127.0.0.1:8003");

    test_helpers::writeFile(configPath, jData.dump(4));

    auto config = std::make_shared<config::JsonConfig>(configPath, crypto);

    std::vector<std::string> peers;
    config->loadTrustedPeerList(peers);

    EXPECT_EQ(peers.size(), 2);
    EXPECT_EQ(peers[0], "127.0.0.1:8002");
    EXPECT_EQ(peers[1], "127.0.0.1:8003");
}

TEST_F(ConfigTest, EmptyTrustedPeerListSucceeds)
{
    std::string configPath = env->createTestConfig(8001);

    auto config = std::make_shared<config::JsonConfig>(configPath, crypto);

    std::vector<std::string> peers;
    config->loadTrustedPeerList(peers);

    EXPECT_TRUE(peers.empty());
}

TEST_F(ConfigTest, ConfigFieldConversionWorks)
{
    EXPECT_EQ(config::IConfig::configFieldToString(config::ConfigField::HOST), "host");
    EXPECT_EQ(config::IConfig::configFieldToString(config::ConfigField::PORT), "port");
    EXPECT_EQ(config::IConfig::configFieldToString(config::ConfigField::PUBLIC_KEY), "public_key");
    EXPECT_EQ(config::IConfig::configFieldToString(config::ConfigField::PRIVATE_KEY),
              "private_key");

    EXPECT_EQ(config::IConfig::stringToConfigField("host"), config::ConfigField::HOST);
    EXPECT_EQ(config::IConfig::stringToConfigField("port"), config::ConfigField::PORT);
    EXPECT_EQ(config::IConfig::stringToConfigField("public_key"), config::ConfigField::PUBLIC_KEY);
    EXPECT_EQ(config::IConfig::stringToConfigField("private_key"),
              config::ConfigField::PRIVATE_KEY);
}

TEST_F(ConfigTest, InvalidConfigFieldThrows)
{
    EXPECT_THROW({ config::IConfig::stringToConfigField("invalid_field"); }, std::runtime_error);
}