#include "TCPClient.hpp"

#include "DisconnectMessage.hpp"
#include "timestamp.hpp"

TCPClient::TCPClient(const std::shared_ptr<IConfig>& config,
                     const std::shared_ptr<ICrypto>& crypto,
                     const std::shared_ptr<ChatService>& chatService,
                     const std::shared_ptr<PeerService>& peerService,
                     const std::shared_ptr<ConsoleUI>& consoleUI)
    : client(),
      config(config),
      crypto(crypto),
      chatService(chatService),
      peerService(peerService),
      consoleUI(consoleUI)
{
    const std::vector<UserHost>& hosts = peerService->getHosts();

    std::string host = config->get(ConfigField::HOST);
    u_short port = static_cast<u_short>(std::stoi(config->get(ConfigField::PORT)));

    for (const auto& userHost : hosts)
    {
        UserPeer from{ host, port, config->get(ConfigField::PUBLIC_KEY) };
        UserPeer to{ userHost.host, userHost.port, "null" };
        u_int64 timestamp = getTimestamp();

        ConnectMessage message(from, to, timestamp);
        sendMessage(message);
    }
}

TCPClient::~TCPClient()
{
    std::string host = config->get(ConfigField::HOST);
    u_short port = static_cast<u_short>(std::stoi(config->get(ConfigField::PORT)));
    std::string publicKey = config->get(ConfigField::PUBLIC_KEY);

    UserPeer from{ host, port, publicKey };
    const std::vector<UserPeer>& peers = peerService->getPeers();

    for (const auto& peer : peers)
    {
        DisconnectMessage message(from, peer, getTimestamp());
        sendMessage(message);
    }
}

void TCPClient::sendMessage(const Message& message, bool withSecret)
{
    client.connectTo(message.to.host, message.to.port);

    json jMessage;
    if (withSecret)
    {
        const MySendMessage& sendMessage = dynamic_cast<const MySendMessage&>(message);
        sendMessage.serialize(jMessage, config->get(ConfigField::PRIVATE_KEY), crypto);
    }
    else
        message.serialize(jMessage);

    if (client.sendMessage(jMessage.dump()))
    {
        std::string response = client.receiveMessage();

        chatService->handleOutgoingMessage(response);
    }

    client.disconnect();
}
