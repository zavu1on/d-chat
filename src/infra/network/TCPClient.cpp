#include "TCPClient.hpp"

#include "DisconnectionMessage.hpp"
#include "SocketServer.hpp"
#include "timestamp.hpp"

namespace network
{
TCPClient::TCPClient(const std::shared_ptr<config::IConfig>& config,
                     const std::shared_ptr<crypto::ICrypto>& crypto,
                     const std::shared_ptr<chat::ChatService>& chatService,
                     const std::shared_ptr<peer::PeerService>& peerService,
                     const std::shared_ptr<ui::ConsoleUI>& consoleUI)
    : client(),
      config(config),
      crypto(crypto),
      chatService(chatService),
      peerService(peerService),
      consoleUI(consoleUI)
{
    const std::vector<peer::UserHost>& hosts = peerService->getHosts();

    std::string host = config->get(config::ConfigField::HOST);
    u_short port = static_cast<u_short>(std::stoi(config->get(config::ConfigField::PORT)));

    for (const auto& userHost : hosts)
    {
        peer::UserPeer from{ host, port, config->get(config::ConfigField::PUBLIC_KEY) };
        peer::UserPeer to{ userHost.host, userHost.port, "null" };
        u_int64 timestamp = utils::getTimestamp();

        message::ConnectionMessage message(from, to, timestamp);
        sendMessage(message);
    }
}

TCPClient::~TCPClient()
{
    std::string host = config->get(config::ConfigField::HOST);
    u_short port = static_cast<u_short>(std::stoi(config->get(config::ConfigField::PORT)));
    std::string publicKey = config->get(config::ConfigField::PUBLIC_KEY);

    peer::UserPeer from{ host, port, publicKey };
    const std::vector<peer::UserPeer>& peers = peerService->getPeers();

    for (const auto& peer : peers)
    {
        message::DisconnectionMessage message(from, peer, utils::getTimestamp());
        sendMessage(message);
    }
}

void TCPClient::sendMessage(const message::Message& message, bool withSecret)
{
    client.connectTo(message.to.host, message.to.port);

    json jMessage;
    if (withSecret)
    {
        const message::TextMessage& sendMessage =
            dynamic_cast<const message::TextMessage&>(message);
        sendMessage.serialize(jMessage, config->get(config::ConfigField::PRIVATE_KEY), crypto);
    }
    else
        message.serialize(jMessage);

    if (jMessage.size() > BUFFER_SIZE) throw std::runtime_error("Message is too big");

    if (client.sendMessage(jMessage.dump()))
    {
        std::string response = client.receiveMessage();

        chatService->handleOutgoingMessage(response);
    }

    client.disconnect();
}
}  // namespace network