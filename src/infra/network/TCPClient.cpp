#include "TCPClient.hpp"

#include "DisconnectionMessage.hpp"
#include "SocketServer.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

namespace network
{
TCPClient::TCPClient(const std::shared_ptr<config::IConfig>& config,
                     const std::shared_ptr<crypto::ICrypto>& crypto,
                     const std::shared_ptr<chat::ChatService>& chatService,
                     const std::shared_ptr<peer::PeerService>& peerService,
                     const std::shared_ptr<blockchain::BlockchainService>& blockchainService,
                     const std::shared_ptr<message::MessageService>& messageService,
                     const std::shared_ptr<ui::ConsoleUI>& consoleUI,
                     const std::string& lastHash)
    : config(config),
      crypto(crypto),
      chatService(chatService),
      peerService(peerService),
      blockchainService(blockchainService),
      messageService(messageService),
      consoleUI(consoleUI)
{
    std::vector<peer::UserHost> hosts = peerService->getHosts();

    std::string host = config->get(config::ConfigField::HOST);
    u_short port = static_cast<u_short>(std::stoi(config->get(config::ConfigField::PORT)));
    peer::UserPeer from{ host, port, config->get(config::ConfigField::PUBLIC_KEY) };

    for (const auto& userHost : hosts)
    {
        peer::UserPeer to{ userHost.host, userHost.port, "" };

        message::ConnectionMessage message = message::ConnectionMessage::create(from, to, lastHash);
        sendMessage(message);
    }
}

void TCPClient::connectToAllPeers()
{
    std::vector<peer::UserPeer> peers = peerService->getPeers();

    std::string host = config->get(config::ConfigField::HOST);
    u_short port = static_cast<u_short>(std::stoi(config->get(config::ConfigField::PORT)));
    peer::UserPeer from{ host, port, config->get(config::ConfigField::PUBLIC_KEY) };

    for (const auto& to : peers)
    {
        message::ConnectionMessage message = message::ConnectionMessage::create(from, to, "0");
        sendMessage(message);
    }
}

void TCPClient::sendMessage(const message::Message& message)
{
    if (typeid(message) == typeid(message::SecretMessage))
        throw std::runtime_error("Secret messages should be sent by sendBlock() method");

    peer::UserPeer to = message.getTo();
    SocketClient client;
    client.connectTo(to.host, to.port);

    json jMessage;
    message.serialize(jMessage);

    if (jMessage.size() > BUFFER_SIZE) throw std::runtime_error("Message is too big");

    if (client.sendMessage(jMessage.dump()))
    {
        std::string response = client.receiveMessage();
        chatService->handleOutgoingMessage(response);
    }

    client.disconnect();
}

void TCPClient::sendSecretMessage(const message::SecretMessage& message)
{
    peer::UserPeer to = message.getTo();
    SocketClient client;
    client.connectTo(to.host, to.port);

    json jMessage;
    message::TextMessage& textMessage =
        const_cast<message::TextMessage&>(dynamic_cast<const message::TextMessage&>(message));

    if (jMessage.size() > BUFFER_SIZE) throw std::runtime_error("Message is too big");

    blockchain::Block block;
    blockchainService->createBlockFromMessage(textMessage, block);
    textMessage.setBlockHash(block.hash);

    textMessage.serialize(jMessage, config->get(config::ConfigField::PRIVATE_KEY), crypto);
    std::string serializedMessage = jMessage.dump();

    if (client.sendMessage(serializedMessage))
    {
        std::string response = client.receiveMessage();
        chatService->handleOutgoingMessage(response);
    }
    client.disconnect();

    std::vector<peer::UserPeer> peers = peerService->getPeers();

    auto sendCallback = [this](const std::string& raw, const peer::UserPeer& peer) -> bool
    {
        SocketClient client;
        client.connectTo(peer.host, peer.port);
        bool ok = client.sendMessage(raw);
        client.disconnect();
        return ok;
    };

    bool stored = blockchainService->storeAndBroadcastBlock(block, peers, sendCallback);
    if (stored)
    {
        messageService->insertSecretMessage(textMessage, serializedMessage, block.hash);
        peerService->addChatPeer(textMessage.getTo());
    }
    else
        consoleUI->printLog("[WARN] block was not stored (maybe duplicate)");
}

void TCPClient::disconnect()
{
    std::string host = config->get(config::ConfigField::HOST);
    u_short port = static_cast<u_short>(std::stoi(config->get(config::ConfigField::PORT)));
    std::string publicKey = config->get(config::ConfigField::PUBLIC_KEY);

    peer::UserPeer from{ host, port, publicKey };
    std::vector<peer::UserPeer> peers = peerService->getPeers();

    for (const auto& peer : peers)
    {
        message::DisconnectionMessage message = message::DisconnectionMessage::create(from, peer);
        sendMessage(message);
    }
}
}  // namespace network