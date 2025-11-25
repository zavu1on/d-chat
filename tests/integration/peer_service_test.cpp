#include <gtest/gtest.h>

#include "DBFile.hpp"
#include "OpenSSLCrypto.hpp"
#include "PeerDB.hpp"
#include "PeerService.hpp"
#include "test_helpers.hpp"


class PeerServiceTest : public ::testing::Test
{
protected:
    std::unique_ptr<test_helpers::TestEnvironment> env;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<db::DBFile> db;
    std::shared_ptr<peer::IPeerRepo> peerRepo;
    std::shared_ptr<peer::PeerService> peerService;

    void SetUp() override
    {
        env = std::make_unique<test_helpers::TestEnvironment>();
        crypto = std::make_shared<crypto::OpenSSLCrypto>();

        std::string dbPath = env->createTestDatabase("peer_test");
        db = std::make_shared<db::DBFile>(dbPath);

        peerRepo = std::make_shared<peer::PeerDB>(db);
        peerRepo->init();

        std::vector<std::string> hosts = { "127.0.0.1:8001", "127.0.0.1:8002" };

        peerService = std::make_shared<peer::PeerService>(hosts, peerRepo);
    }

    void TearDown() override
    {
        db->close();
        env->cleanup();
    }
};

TEST_F(PeerServiceTest, GetHostsReturnsConfiguredHosts)
{
    const auto& hosts = peerService->getHosts();

    ASSERT_EQ(hosts.size(), 2);
    EXPECT_EQ(hosts[0].host, "127.0.0.1");
    EXPECT_EQ(hosts[0].port, 8001);
    EXPECT_EQ(hosts[1].host, "127.0.0.1");
    EXPECT_EQ(hosts[1].port, 8002);
}

TEST_F(PeerServiceTest, AddPeerAndRetrieveSucceeds)
{
    peer::UserPeer peer = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);

    peerService->addPeer(peer);

    auto peers = peerService->getPeers();
    ASSERT_EQ(peers.size(), 1);
    EXPECT_EQ(peers[0].host, peer.host);
    EXPECT_EQ(peers[0].port, peer.port);
    EXPECT_EQ(peers[0].publicKey, peer.publicKey);
}

TEST_F(PeerServiceTest, AddDuplicatePeerDoesNotCreateDuplicate)
{
    peer::UserPeer peer = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);

    peerService->addPeer(peer);
    peerService->addPeer(peer);

    auto peers = peerService->getPeers();
    EXPECT_EQ(peers.size(), 1);
}

TEST_F(PeerServiceTest, RemovePeerSucceeds)
{
    peer::UserPeer peer1 = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);
    peer::UserPeer peer2 = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER2, crypto);

    peerService->addPeer(peer1);
    peerService->addPeer(peer2);

    EXPECT_EQ(peerService->getPeersCount(), 2);

    peerService->removePeer(peer1);

    EXPECT_EQ(peerService->getPeersCount(), 1);

    auto peers = peerService->getPeers();
    EXPECT_EQ(peers[0].publicKey, peer2.publicKey);
}

TEST_F(PeerServiceTest, FindPeerByHostSucceeds)
{
    peer::UserPeer peer = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);

    peerService->addPeer(peer);

    peer::UserHost host(peer.host, peer.port);
    peer::UserPeer found = peerService->findPeer(host);

    EXPECT_EQ(found.host, peer.host);
    EXPECT_EQ(found.port, peer.port);
    EXPECT_EQ(found.publicKey, peer.publicKey);
}

TEST_F(PeerServiceTest, FindNonExistentPeerThrows)
{
    peer::UserHost nonExistent("127.0.0.1", 9999);

    EXPECT_THROW({ peerService->findPeer(nonExistent); }, std::range_error);
}

TEST_F(PeerServiceTest, AddChatPeerPersistsToDatabase)
{
    peer::UserPeer peer = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);

    peerService->addChatPeer(peer);

    std::vector<peer::UserPeer> chatPeers;
    peerService->getAllChatPeers(chatPeers);

    ASSERT_EQ(chatPeers.size(), 1);
    EXPECT_EQ(chatPeers[0].publicKey, peer.publicKey);
}

TEST_F(PeerServiceTest, FindPublicKeyByUserHostSucceeds)
{
    peer::UserPeer peer = test_helpers::createTestPeer(test_helpers::TEST_PORT_PEER1, crypto);

    peerService->addChatPeer(peer);

    peer::UserHost host(peer.host, peer.port);
    std::string publicKey;

    EXPECT_TRUE(peerService->findPublicKeyByUserHost(host, publicKey));
    EXPECT_EQ(publicKey, peer.publicKey);
}

TEST_F(PeerServiceTest, FindPublicKeyForNonExistentPeerReturnsFalse)
{
    peer::UserHost nonExistent("127.0.0.1", 9999);
    std::string publicKey;

    EXPECT_FALSE(peerService->findPublicKeyByUserHost(nonExistent, publicKey));
}