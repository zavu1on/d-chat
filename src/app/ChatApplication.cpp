#include "ChatApplication.hpp"

ChatApplication::~ChatApplication()
{
    consoleUI->stop();
    server->stop();
}

void ChatApplication::init()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    running.store(false, std::memory_order_relaxed);

    consoleUI = std::make_shared<ConsoleUI>();

    crypto = std::make_shared<OpenSSLCrypto>();
    config = std::make_shared<JsonConfig>("d-chat_config.json", crypto);
    if (!config->isValid())
    {
        consoleUI->printLog("[WARN] Default config was generated\n");
        config->generatedDefaultConfig();
    }
    u_short port = static_cast<u_short>(std::stoi(config->get(ConfigField::PORT)));

    std::vector<std::string> peerList;
    config->loadTrustedPeerList(peerList);
    if (peerList.empty())
        consoleUI->printLog("[WARN] No trusted peers found in config. Can not connect to anyone\n");

    peerService = std::make_shared<PeerService>(peerList);
    chatService = std::make_shared<ChatService>(config, crypto, peerService, consoleUI);

    server = std::make_shared<TCPServer>(port, chatService);
    client = std::make_shared<TCPClient>(config, crypto, chatService, peerService, consoleUI);
}

void ChatApplication::run()
{
    server->start();
    consoleUI->printLog("[INFO] Server started\n");

    consoleUI->startInputLoop(
        [this](const std::string& input)
        {
            if (input == "exit")
            {
                running.store(false, std::memory_order_relaxed);
                consoleUI->printLog("[SYSTEM] Shutting down...");
                consoleUI->stop();
                return;
            }

            consoleUI->printLog("[YOU] " + input);
        });

    running.store(true, std::memory_order_relaxed);

    while (running.load(std::memory_order_relaxed))
    {
    }
}
