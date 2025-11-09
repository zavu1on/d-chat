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

class ChatApplication
{
private:
    std::shared_ptr<ConsoleUI> consoleUI;
    std::shared_ptr<ICrypto> crypto;
    std::shared_ptr<IConfig> config;
    std::shared_ptr<PeerService> peerService;
    std::shared_ptr<ChatService> chatService;
    std::shared_ptr<IChatServer> server;
    std::shared_ptr<IChatClient> client;

    std::atomic<bool> running;

public:
    ~ChatApplication();

    void init();
    void run();
};
