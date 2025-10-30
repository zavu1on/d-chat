#pragma once

#include <iostream>
#include <memory>
#include <thread>

#include "ChatService.hpp"
#include "TCPClient.hpp"
#include "TCPServer.hpp"

class ChatApplication
{
private:
    std::shared_ptr<ChatService> chatService;
    std::unique_ptr<TCPServer> server;
    std::unique_ptr<TCPClient> client;
    std::thread serverThread;
    bool running = false;

public:
    ChatApplication() = default;
    ~ChatApplication();

    void init();
    void run();
    void shutdown();
};
