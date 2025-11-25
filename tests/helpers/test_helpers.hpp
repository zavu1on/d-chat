#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "Block.hpp"
#include "DBFile.hpp"
#include "IConfig.hpp"
#include "ICrypto.hpp"
#include "OpenSSLCrypto.hpp"
#include "UserPeer.hpp"

namespace test_helpers
{
constexpr unsigned short TEST_PORT_BASE = 18000;
constexpr unsigned short TEST_PORT_PEER1 = TEST_PORT_BASE + 1;
constexpr unsigned short TEST_PORT_PEER2 = TEST_PORT_BASE + 2;
constexpr unsigned short TEST_PORT_PEER3 = TEST_PORT_BASE + 3;

class TestEnvironment
{
private:
    std::string testDir;
    std::vector<std::string> createdFiles;

public:
    TestEnvironment();
    ~TestEnvironment();

    std::string getTestDir() const { return testDir; }
    std::string createTempFile(const std::string& name);
    std::string createTestConfig(unsigned short port,
                                 const std::string& privateKey = "",
                                 const std::string& publicKey = "");
    std::string createTestDatabase(const std::string& name);
    void cleanup();
};

peer::UserPeer createTestPeer(unsigned short port, const std::shared_ptr<crypto::ICrypto>& crypto);

blockchain::Block createTestBlock(const std::string& previousHash,
                                  const std::string& payloadHash,
                                  const std::shared_ptr<crypto::ICrypto>& crypto,
                                  const crypto::Bytes& privateKey);

bool waitForPort(unsigned short port, int timeoutMs = 5000);

bool fileExists(const std::string& path);

std::string readFile(const std::string& path);

void writeFile(const std::string& path, const std::string& content);
}  // namespace test_helpers