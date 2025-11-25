#include "test_helpers.hpp"

#include <winsock2.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <thread>

#include "JsonFile.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"


namespace test_helpers
{

TestEnvironment::TestEnvironment()
{
    std::srand(std::time(nullptr));
    testDir =
        "test_env_" + std::to_string(std::rand()) + "_" + std::to_string(utils::getTimestamp());
    std::filesystem::create_directories(testDir);
}

TestEnvironment::~TestEnvironment() { cleanup(); }

std::string TestEnvironment::createTempFile(const std::string& name)
{
    std::string path = testDir + "/" + name;
    createdFiles.push_back(path);
    return path;
}

std::string TestEnvironment::createTestConfig(unsigned short port,
                                              const std::string& privateKey,
                                              const std::string& publicKey)
{
    std::string configPath = createTempFile("test_config_" + std::to_string(port) + ".json");

    nlohmann::json jData;
    jData["host"] = "127.0.0.1";
    jData["port"] = std::to_string(port);
    jData["private_key"] = privateKey.empty() ? "test_private_key" : privateKey;
    jData["public_key"] = publicKey.empty() ? "test_public_key" : publicKey;
    jData["trustedPeerList"] = nlohmann::json::array();

    std::ofstream file(configPath);
    file << jData.dump(4);
    file.close();

    return configPath;
}

std::string TestEnvironment::createTestDatabase(const std::string& name)
{
    std::string dbPath = createTempFile(name + ".db");
    return dbPath;
}

void TestEnvironment::cleanup()
{
    for (const auto& file : createdFiles)
    {
        try
        {
            std::filesystem::remove(file);
        }
        catch (...)
        {
        }
    }

    try
    {
        std::filesystem::remove_all(testDir);
    }
    catch (...)
    {
    }
}

peer::UserPeer createTestPeer(unsigned short port, const std::shared_ptr<crypto::ICrypto>& crypto)
{
    auto keyPair = crypto->generateKeyPair();
    return peer::UserPeer("127.0.0.1", port, crypto->keyToString(keyPair.publicKey));
}

blockchain::Block createTestBlock(const std::string& previousHash,
                                  const std::string& payloadHash,
                                  const std::shared_ptr<crypto::ICrypto>& crypto,
                                  const crypto::Bytes& privateKey)
{
    blockchain::Block block;
    block.previousHash = previousHash;
    block.payloadHash = payloadHash;
    block.authorPublicKey = "test_author";
    block.timestamp = utils::getTimestamp();

    std::string canonical = block.toStringForHash();
    crypto::Bytes canonicalBytes(canonical.begin(), canonical.end());
    crypto::Bytes signature = crypto->sign(canonicalBytes, privateKey);
    block.signature = crypto->keyToString(signature);

    block.computeHash();
    return block;
}

bool waitForPort(unsigned short port, int timeoutMs)
{
    auto start = std::chrono::steady_clock::now();

    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                 start)
               .count() < timeoutMs)
    {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) continue;

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0)
        {
            closesocket(sock);
            return true;
        }

        closesocket(sock);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}

bool fileExists(const std::string& path) { return std::filesystem::exists(path); }

std::string readFile(const std::string& path)
{
    std::ifstream file(path);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void writeFile(const std::string& path, const std::string& content)
{
    std::ofstream file(path);
    file << content;
}
}  // namespace test_helpers