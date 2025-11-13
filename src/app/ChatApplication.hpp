#pragma once
#include <OpenSSLCrypto.hpp>
#include <XORCrypto.hpp>
#include <iostream>
#include <memory>

#include "ChatService.hpp"
#include "ConsoleUI.hpp"
#include "JsonConfig.hpp"
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
    std::shared_ptr<network::IChatServer> server;
    std::shared_ptr<network::IChatClient> client;

    peer::UserPeer from;
    std::atomic<bool> running;

public:
    ~ChatApplication();

    void init();
    void run();
};
}  // namespace app