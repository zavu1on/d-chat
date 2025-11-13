#include "ChatService.hpp"

#include "timestamp.hpp"

void ChatService::handleIncomingConnectMessage(const ConnectMessage& message, std::string& response)
{
    peerService->addPeer({ message.from.host, message.from.port, message.from.publicKey });

    UserPeer me{ message.to.host, message.to.port, config->get(ConfigField::PUBLIC_KEY) };
    uint64_t timestamp = getTimestamp();

    ConnectResponseMessage responseMessage(me, message.from, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
    consoleUI->printLog("[SERVER] accepted new peer from " + message.from.host + ":" +
                        std::to_string(message.from.port) + "\n");
}

void ChatService::handleOutgoingConnectMessage(const ConnectResponseMessage& response)
{
    peerService->addPeer({ response.from.host, response.from.port, response.from.publicKey });
}

void ChatService::handleIncomingMessage(const SendMessage& message, std::string& response)
{
    consoleUI->printLog("[SERVER] received message from " + message.from.host + ":" +
                        std::to_string(message.from.port) + ": " + message.payload.message + "\n");

    UserPeer me{ message.to.host, message.to.port, config->get(ConfigField::PUBLIC_KEY) };
    uint64_t timestamp = getTimestamp();

    SendResponseMessage responseMessage(me, message.from, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
}

void ChatService::handleOutgoingMessage(const SendResponseMessage& response) { (void)response; }

void ChatService::handleIncomingDisconnectMessage(const DisconnectMessage& message,
                                                  std::string& response)
{
    peerService->removePeer(message.from);

    UserPeer from = message.to;
    UserPeer to = message.from;
    uint64_t timestamp = getTimestamp();
    DisconnectResponseMessage responseMessage(from, to, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
    consoleUI->printLog("[SERVER] removed peer " + message.from.host + ":" +
                        std::to_string(message.from.port) + "\n");
}

void ChatService::handleOutgoingDisconnectMessage(const DisconnectResponseMessage& response)
{
    (void)response;
}

ChatService::ChatService(const std::shared_ptr<IConfig>& config,
                         const std::shared_ptr<ICrypto>& crypto,
                         const std::shared_ptr<PeerService>& peerService,
                         const std::shared_ptr<ConsoleUI>& consoleUI)
    : config(config), crypto(crypto), peerService(peerService), consoleUI(consoleUI)
{
}

void ChatService::handleIncomingMessage(const std::string& message, std::string& response)
{
    try
    {
        json jData = json::parse(message);

        if (jData["type"] == Message::fromMessageTypeToString(MessageType::CONNECT))
        {
            ConnectMessage message(jData);
            handleIncomingConnectMessage(message, response);
        }
        else if (jData["type"] == Message::fromMessageTypeToString(MessageType::SEND_MESSAGE))
        {
            try
            {
                SendMessage message(jData, config->get(ConfigField::PRIVATE_KEY), crypto);
                handleIncomingMessage(message, response);
            }
            catch (const std::exception& e)
            {
                consoleUI->printLog("[SERVER] error while handleing incoming message: " +
                                    std::string(e.what()));
            }
        }
        else if (jData["type"] == Message::fromMessageTypeToString(MessageType::DISCONNECT))
        {
            DisconnectMessage message(jData);
            handleIncomingDisconnectMessage(message, response);
        }
    }
    catch (json::exception& e)
    {
        consoleUI->printLog("[SERVER] handle incoming message error: " + std::string(e.what()));
    }
}

void ChatService::handleOutgoingMessage(const std::string& response)
{
    try
    {
        json jData = json::parse(response);

        if (jData["type"] == Message::fromMessageTypeToString(MessageType::CONNECT_RESPONSE))
        {
            ConnectResponseMessage response(jData);
            handleOutgoingConnectMessage(response);
        }
        else if (jData["type"] == Message::fromMessageTypeToString(MessageType::SEND_MESSAGE))
        {
            SendResponseMessage response(jData);
            handleOutgoingMessage(response);
        }
        else if (jData["type"] ==
                 Message::fromMessageTypeToString(MessageType::DISCONNECT_RESPONSE))
        {
            DisconnectResponseMessage response(jData);
            handleOutgoingDisconnectMessage(response);
        }
    }
    catch (json::exception& e)
    {
        consoleUI->printLog("[CLIENT] handle outgoing message error: " + std::string(e.what()));
    }
}
