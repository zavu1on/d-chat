#include "ChatApplication.hpp"

#include "SendMessage.hpp"
#include "timestamp.hpp"

ChatApplication::~ChatApplication() { server->stop(); }

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

    from = UserPeer(config->get(ConfigField::HOST), port, config->get(ConfigField::PUBLIC_KEY));

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
            if (input == "/exit")
            {
                running.store(false, std::memory_order_relaxed);
                consoleUI->printLog("[SYSTEM] Shutting down...");
                consoleUI->stop();
                return;
            }

            if (input.substr(0, 5) == "/send")
            {
                try
                {
                    std::string command = input.substr(5);
                    size_t start = command.find_first_not_of(" \t");
                    if (start == std::string::npos)
                    {
                        consoleUI->printLog(
                            "[ERROR] Invalid command format. Usage: /send <host:port> <message>");
                    }
                    command = command.substr(start);

                    size_t spacePos = command.find(' ');
                    if (spacePos == std::string::npos)
                    {
                        consoleUI->printLog(
                            "[ERROR] Invalid command format. Usage: /send <host:port> <message>");
                    }

                    std::string hostPort = command.substr(0, spacePos);
                    std::string message = command.substr(spacePos + 1);

                    size_t colonPos = hostPort.find(':');
                    if (colonPos == std::string::npos)
                    {
                        consoleUI->printLog(
                            "[ERROR] Invalid host:port format. Usage: /send <host:port> <message>");
                    }

                    std::string host = hostPort.substr(0, colonPos);
                    std::string portStr = hostPort.substr(colonPos + 1);

                    if (portStr.empty() || !std::all_of(portStr.begin(), portStr.end(), ::isdigit))
                    {
                        consoleUI->printLog(
                            "[ERROR] Port must be a number. Usage: /send <host:port> <message>");
                    }

                    int port = std::stoi(portStr);

                    UserPeer to = peerService->findPeer(UserHost(host, port));
                    uint64_t timestamp = getTimestamp();

                    MySendMessage sendMessage(from, to, timestamp, message);
                    client->sendMessage(sendMessage, true);
                }
                catch (std::exception& e)
                {
                    consoleUI->printLog("[ERROR] " + std::string(e.what()));
                }
            }
            else
                consoleUI->printLog("[YOU] " + input);
        });

    running.store(true, std::memory_order_relaxed);

    while (running.load(std::memory_order_relaxed))
    {
    }
}

// /send 127.0.0.1:8001 Hi