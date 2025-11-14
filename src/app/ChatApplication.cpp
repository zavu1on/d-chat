#include "ChatApplication.hpp"

#include "GlobalState.hpp"
#include "TextMessage.hpp"
#include "timestamp.hpp"

namespace app
{
const u_int PEERS_BATCH_SIZE = 4;

ChatApplication::~ChatApplication()
{
    server->stop();
    client->disconnect();
}

void ChatApplication::init()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, "ru_RU.UTF-8");
    running.store(false, std::memory_order_relaxed);

    consoleUI = std::make_shared<ui::ConsoleUI>();

    crypto = std::make_shared<crypto::OpenSSLCrypto>();
    config = std::make_shared<config::JsonConfig>("d-chat_config.json", crypto);
    if (!config->isValid())
    {
        consoleUI->printLog("[WARN] Default config was generated\n");
        config->generatedDefaultConfig();
    }
    u_short port = static_cast<u_short>(std::stoi(config->get(config::ConfigField::PORT)));

    from = peer::UserPeer(
        config->get(config::ConfigField::HOST), port, config->get(config::ConfigField::PUBLIC_KEY));

    std::vector<std::string> peerList;
    config->loadTrustedPeerList(peerList);
    if (peerList.empty())
        consoleUI->printLog("[WARN] No trusted peers found in config. Can not connect to anyone\n");

    peerService = std::make_shared<peer::PeerService>(peerList);
    chatService = std::make_shared<chat::ChatService>(config, crypto, peerService, consoleUI);

    server = std::make_shared<network::TCPServer>(port, chatService);
    client =
        std::make_shared<network::TCPClient>(config, crypto, chatService, peerService, consoleUI);

    if (peerService->getPeersCount() > 0)
    {
        config::GlobalState& globalState = config::GlobalState::getInstance();
        u_int peersToReceive = globalState.getPeersToReceive();
        peer::UserPeer to = peerService->getPeer(0);
        u_int start = 0;
        u_int end = peersToReceive > PEERS_BATCH_SIZE ? PEERS_BATCH_SIZE : peersToReceive;

        do
        {
            message::PeerListMessage message(from, to, utils::getTimestamp(), start, end);
            client->sendMessage(message, false);
            start += PEERS_BATCH_SIZE;
            end += PEERS_BATCH_SIZE;
        } while (end < peersToReceive);
    }

    client->connectToAllPeers();
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

                    peer::UserPeer to = peerService->findPeer(peer::UserHost(host, port));
                    uint64_t timestamp = utils::getTimestamp();

                    message::TextMessage sendMessage(from, to, timestamp, message);
                    client->sendMessage(sendMessage, true);
                }
                catch (std::exception& error)
                {
                    consoleUI->printLog("[ERROR] " + std::string(error.what()));
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
}  // namespace app