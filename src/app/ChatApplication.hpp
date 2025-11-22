#pragma once
#include <OpenSSLCrypto.hpp>
#include <XORCrypto.hpp>
#include <iostream>
#include <memory>

#include "BlockchainService.hpp"
#include "ChainDB.hpp"
#include "ChatService.hpp"
#include "ConsoleUI.hpp"
#include "DBFile.hpp"
#include "JsonConfig.hpp"
#include "MessageDB.hpp"
#include "MessageService.hpp"
#include "PeerDB.hpp"
#include "PeerService.hpp"
#include "TCPClient.hpp"
#include "TCPServer.hpp"

namespace app
{
class ChatApplication
{
private:
    std::shared_ptr<ui::ConsoleUI> consoleUI;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<peer::PeerService> peerService;
    std::shared_ptr<chat::ChatService> chatService;
    std::shared_ptr<blockchain::BlockchainService> blockchainService;
    std::shared_ptr<message::MessageService> messageService;
    std::shared_ptr<network::IChatServer> server;
    std::shared_ptr<network::IChatClient> client;
    std::shared_ptr<db::DBFile> db;
    std::shared_ptr<blockchain::IChainRepo> chainRepo;
    std::shared_ptr<message::IMessageRepo> messageRepo;
    std::shared_ptr<peer::IPeerRepo> peerRepo;
    peer::UserPeer from;
    std::atomic<bool> running;

    void handlePeersCommand();
    void handleChatsCommand();
    void handleChatCommand(const std::string& args);
    void handleSendCommand(const std::string& args);
    void handleHelpCommand();

public:
    ~ChatApplication();
    void init();
    void run();
};
}  // namespace app