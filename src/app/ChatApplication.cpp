#include "ChatApplication.hpp"

#include <unordered_set>

#include "GlobalState.hpp"
#include "TextMessage.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

namespace app
{
constexpr const u_int PEERS_BATCH_SIZE = 12;
constexpr const u_int BLOCKS_BATCH_SIZE = 4;
constexpr const char* DB_PATH = "d-chat.db";

void ChatApplication::handlePeersCommand()
{
    std::vector<peer::UserPeer> peers = peerService->getPeers();

    if (peers.empty())
    {
        consoleUI->printLog("[INFO] No connected peers");
        return;
    }

    std::string output = "[PEERS LIST]\n";
    output += "Total peers: " + std::to_string(peers.size()) + "\n";

    for (size_t i = 0; i < peers.size(); ++i)
    {
        output += std::to_string(i + 1) + ". " + peers[i].host + ":" +
                  std::to_string(peers[i].port) + "\n";
        output += "   Public Key: " + peers[i].publicKey.substr(0, 8) + "...\n";
    }

    consoleUI->printLog(output);
}

void ChatApplication::handleChatsCommand()
{
    std::vector<peer::UserPeer> chatPeers;
    peerService->getAllChatPeers(chatPeers);

    if (chatPeers.empty())
    {
        consoleUI->printLog("[INFO] No chat peers found\n");
        return;
    }

    std::string output = "[CHAT PEERS LIST]\n";
    output += "Total chats: " + std::to_string(chatPeers.size()) + "\n";

    for (size_t i = 0; i < chatPeers.size(); ++i)
    {
        output += std::to_string(i + 1) + ". " + chatPeers[i].host + ":" +
                  std::to_string(chatPeers[i].port) + "\n";
        output += "   Public Key: " + chatPeers[i].publicKey.substr(0, 8) + "...\n";
    }

    consoleUI->printLog(output);
}

void ChatApplication::handleSendCommand(const std::string& args)
{
    try
    {
        std::string command = args;
        size_t start = command.find_first_not_of(" \t");
        if (start == std::string::npos)
        {
            consoleUI->printLog(
                "[ERROR] Invalid command format. Usage: /send <host:port> <message> or /send all "
                "<message>\n");
            return;
        }
        command = command.substr(start);

        if (command.substr(0, 3) == "all")
        {
            size_t spaceAfterAll = command.find(' ', 3);
            if (spaceAfterAll == std::string::npos)
            {
                consoleUI->printLog("[ERROR] Invalid command format. Usage: /send all <message>\n");
                return;
            }

            std::string message = command.substr(spaceAfterAll + 1);
            if (message.empty())
            {
                consoleUI->printLog("[ERROR] Message cannot be empty\n");
                return;
            }

            std::vector<peer::UserPeer> peers = peerService->getPeers();
            if (peers.empty())
            {
                consoleUI->printLog("[INFO] No online peers to send message to\n");
                return;
            }

            uint64_t timestamp = utils::getTimestamp();
            std::string errors;

            for (const auto& peer : peers)
            {
                try
                {
                    message::TextMessage sendMessage(
                        utils::uuidv4(), from, peer, timestamp, message, "0");
                    client->sendSecretMessage(sendMessage);
                }
                catch (const std::exception& error)
                {
                    errors += "\n  to " + peer.host + ":" + std::to_string(peer.port) + " - " +
                              error.what();
                }
            }
        }
        else
        {
            size_t spacePos = command.find(' ');
            if (spacePos == std::string::npos)
            {
                consoleUI->printLog(
                    "[ERROR] Invalid command format. Usage: /send <host:port> <message> or /send "
                    "all <message>\n");
                return;
            }

            std::string hostPort = command.substr(0, spacePos);
            std::string message = command.substr(spacePos + 1);

            size_t colonPos = hostPort.find(':');
            if (colonPos == std::string::npos)
            {
                consoleUI->printLog(
                    "[ERROR] Invalid host:port format. Usage: /send <host:port> <message> or /send "
                    "all <message>\n");
                return;
            }

            std::string host = hostPort.substr(0, colonPos);
            std::string portStr = hostPort.substr(colonPos + 1);

            if (portStr.empty() || !std::all_of(portStr.begin(), portStr.end(), ::isdigit))
            {
                consoleUI->printLog(
                    "[ERROR] Port must be a number. Usage: /send <host:port> <message> or /send "
                    "all <message>\n");
                return;
            }

            int port = std::stoi(portStr);

            peer::UserPeer to = peerService->findPeer(peer::UserHost(host, port));
            uint64_t timestamp = utils::getTimestamp();

            message::TextMessage sendMessage(utils::uuidv4(), from, to, timestamp, message, "0");
            client->sendSecretMessage(sendMessage);
            consoleUI->printLog("[INFO] Message sent to " + host + ":" + std::to_string(port) +
                                "\n");
        }
    }
    catch (std::exception& error)
    {
        consoleUI->printLog("[ERROR] " + std::string(error.what()) + "\n");
    }
}

void ChatApplication::handleChatCommand(const std::string& args)
{
    std::string hostPort = args;

    hostPort.erase(0, hostPort.find_first_not_of(" \t"));
    hostPort.erase(hostPort.find_last_not_of(" \t") + 1);

    if (hostPort.empty())
    {
        consoleUI->printLog("[ERROR] Please specify a peer host:port. Usage: /chat <host:port>\n");
        return;
    }

    try
    {
        size_t colonPos = hostPort.find(':');
        if (colonPos == std::string::npos)
        {
            consoleUI->printLog("[ERROR] Invalid host:port format. Usage: /chat <host:port>\n");
            return;
        }

        std::string host = hostPort.substr(0, colonPos);
        std::string portStr = hostPort.substr(colonPos + 1);

        if (portStr.empty() || !std::all_of(portStr.begin(), portStr.end(), ::isdigit))
        {
            consoleUI->printLog("[ERROR] Port must be a number. Usage: /chat <host:port>\n");
            return;
        }

        unsigned short port = static_cast<unsigned short>(std::stoi(portStr));
        peer::UserHost userHost(host, port);

        std::string peerPublicKey;
        if (!peerService->findPublicKeyByUserHost(userHost, peerPublicKey))
        {
            consoleUI->printLog("[ERROR] No peer found with address " + host + ":" +
                                std::to_string(port) + ". Try /chats to see available chats.\n");
            return;
        }

        std::string myPublicKey = config->get(config::ConfigField::PUBLIC_KEY);

        std::vector<message::TextMessage> messages;
        messageService->findChatMessages(myPublicKey, peerPublicKey, messages);

        if (messages.empty())
        {
            consoleUI->printLog("[INFO] No messages found with peer at " + host + ":" +
                                std::to_string(port) + "\n");
            return;
        }

        std::vector<std::string> invalidIds;
        messageService->findInvalidChatMessageIDs(messages, invalidIds);
        std::unordered_set<std::string> invalidSet(invalidIds.begin(), invalidIds.end());

        std::string output = "[CHAT HISTORY with " + host + ":" + std::to_string(port) + "]\n";
        output += "Total messages: " + std::to_string(messages.size());
        if (!invalidIds.empty())
        {
            output += " (" + std::to_string(invalidIds.size()) + " invalid)";
        }
        output += "\n";

        for (const auto& message : messages)
        {
            std::string sender = (message.getFrom().publicKey == myPublicKey) ? "YOU" : "PEER";
            std::string formattedTime = utils::timestampToString(message.getTimestamp());
            std::string status = invalidSet.count(message.getId()) ? " [INVALID]" : "";

            output += "[" + formattedTime + "] " + sender + ": " + message.getPayload().message +
                      status + "\n";
        }

        consoleUI->printLog(output);
    }
    catch (const std::exception& e)
    {
        consoleUI->printLog("[ERROR] Failed to retrieve chat history: " + std::string(e.what()) +
                            "\n");
    }
}

void ChatApplication::handleHelpCommand()
{
    std::string helpMessage =
        "[COMMAND HELP]\n"
        "Available commands:\n"
        "  /help                   - Show this help message\n"
        "  /exit                   - Exit the application\n"
        "  /peers                  - Show list of online peers\n"
        "  /chats                  - Show all your chat conversations\n"
        "  /chat <host:port>       - View chat history with specific peer\n"
        "  /send <host:port> <message> - Send a message to a specific peer\n\n"
        "Examples:\n"
        "  /chat 127.0.0.1:8001\n"
        "  /send 127.0.0.1:8001 Hello, how are you?\n";

    consoleUI->printLog(helpMessage);
}

void ChatApplication::shutdown()
{
    consoleUI->printLog("[SYSTEM] Shutting down...");

    server->stop();
    client->disconnect();
    db->close();

    running.store(false, std::memory_order_relaxed);
}

void ChatApplication::init()
{
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
        consoleUI->printLog(
            "[WARN] No trusted peers found in config. Can not connect to anyone. You will not be "
            "able to send messages (except peers that have connected to you).\n");

    db = std::make_shared<db::DBFile>(DB_PATH);
    chainRepo = std::make_shared<blockchain::ChainDB>(db, config, crypto);
    chainRepo->init();
    messageRepo = std::make_shared<message::MessageDB>(db, config, crypto);
    messageRepo->init();
    peerRepo = std::make_shared<peer::PeerDB>(db);
    peerRepo->init();

    blockchain::Block tip;
    chainRepo->findTip(tip);

    blockchainService =
        std::make_shared<blockchain::BlockchainService>(config, crypto, chainRepo, consoleUI);
    peerService = std::make_shared<peer::PeerService>(peerList, peerRepo);
    messageService = std::make_shared<message::MessageService>(
        messageRepo, blockchainService, config, crypto, consoleUI);
    chatService = std::make_shared<chat::ChatService>(
        config, crypto, peerService, blockchainService, messageService, consoleUI);

    server = std::make_shared<network::TCPServer>(port, chatService, blockchainService, consoleUI);
    client = std::make_shared<network::TCPClient>(config,
                                                  crypto,
                                                  chatService,
                                                  peerService,
                                                  blockchainService,
                                                  messageService,
                                                  consoleUI,
                                                  tip.hash);

    consoleUI->setShutdownCallback([this]() { this->shutdown(); });

    blockchainService->validateLocalChain();

    u_int tipIndex = 0;
    u_int count = 0;
    chainRepo->findTipIndex(tipIndex);

    if (peerService->getPeersCount() > 0)
    {
        config::GlobalState& globalState = config::GlobalState::getInstance();
        u_int peersToReceive = globalState.getPeersToReceive();
        peer::UserPeer to = peerService->getPeer(0);
        u_int start = 0;

        while (start < peersToReceive)
        {
            message::PeerListMessage message(
                utils::uuidv4(), from, to, utils::getTimestamp(), start, PEERS_BATCH_SIZE);
            client->sendMessage(message);
            start += PEERS_BATCH_SIZE;
        }

        u_int blocksToReceive = globalState.getMissingBlocksCount();
        start = 0;
        while (start < blocksToReceive)
        {
            message::BlockRangeMessage message(utils::uuidv4(),
                                               from,
                                               to,
                                               utils::getTimestamp(),
                                               start,
                                               BLOCKS_BATCH_SIZE,
                                               tip.hash);
            client->sendMessage(message);
            start += BLOCKS_BATCH_SIZE;
            count += BLOCKS_BATCH_SIZE;
        }
    }

    if (blockchainService->validateNewBlocks())
    {
        std::vector<blockchain::Block> blocks = blockchainService->getNewBlocks();
        for (const auto& block : blocks)
        {
            chainRepo->insertBlock(block);
        }
    }
    else
    {
        consoleUI->printLog("[ERROR] Can not add new invalid blocks\n");
    }

    client->connectToAllPeers();
}

void ChatApplication::run()
{
    server->start();
    consoleUI->printLog("[INFO] Server started");

    consoleUI->startInputLoop(
        [this](const std::string& input)
        {
            if (input == "/exit")
            {
                this->shutdown();
                consoleUI->stop();

                return;
            }
            else if (input == "/peers")
                handlePeersCommand();
            else if (input == "/chats")
                handleChatsCommand();
            else if (input.substr(0, 5) == "/chat")
            {
                if (input.size() > 6)
                    handleChatCommand(input.substr(5));
                else
                    consoleUI->printLog(
                        "[ERROR] Invalid command format. Usage: /chat <host:port>\n");
            }
            else if (input.substr(0, 5) == "/send")
                handleSendCommand(input.substr(5));
            else if (input.substr(0, 5) == "/help")
                handleHelpCommand();
            else if (!input.empty())
                consoleUI->printLog("[YOU] " + input + "\n");
        });

    running.store(true, std::memory_order_relaxed);
    while (running.load(std::memory_order_relaxed))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

}  // namespace app
