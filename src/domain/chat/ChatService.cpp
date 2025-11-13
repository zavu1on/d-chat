#include "ChatService.hpp"

#include "timestamp.hpp"

namespace chat
{
void ChatService::handleIncomingConnectionMessage(const message::ConnectionMessage& message,
                                                  std::string& response)
{
    peerService->addPeer({ message.from.host, message.from.port, message.from.publicKey });

    peer::UserPeer me{ message.to.host,
                       message.to.port,
                       config->get(config::ConfigField::PUBLIC_KEY) };
    uint64_t timestamp = utils::getTimestamp();

    message::ConnectionMessageResponse responseMessage(me, message.from, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
    consoleUI->printLog("[SERVER] accepted new peer from " + message.from.host + ":" +
                        std::to_string(message.from.port) + "\n");
}

void ChatService::handleOutgoingConnectionMessage(
    const message::ConnectionMessageResponse& response)
{
    peerService->addPeer({ response.from.host, response.from.port, response.from.publicKey });
}

void ChatService::handleIncomingMessage(const message::TextMessage& message, std::string& response)
{
    consoleUI->printLog("[SERVER] received message from " + message.from.host + ":" +
                        std::to_string(message.from.port) + ": " + message.payload.message + "\n");

    peer::UserPeer me{ message.to.host,
                       message.to.port,
                       config->get(config::ConfigField::PUBLIC_KEY) };
    uint64_t timestamp = utils::getTimestamp();

    message::TextMessageResponse responseMessage(me, message.from, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
}

void ChatService::handleOutgoingMessage(const message::TextMessageResponse& response)
{
    (void)response;
}

void ChatService::handleIncomingDisconnectionMessage(const message::DisconnectionMessage& message,
                                                     std::string& response)
{
    peerService->removePeer(message.from);

    peer::UserPeer from = message.to;
    peer::UserPeer to = message.from;
    uint64_t timestamp = utils::getTimestamp();
    message::DisconnectionMessageResponse responseMessage(from, to, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
    consoleUI->printLog("[SERVER] removed peer " + message.from.host + ":" +
                        std::to_string(message.from.port) + "\n");
}

void ChatService::handleOutgoingDisconnectionMessage(
    const message::DisconnectionMessageResponse& response)
{
    (void)response;
}

ChatService::ChatService(const std::shared_ptr<config::IConfig>& config,
                         const std::shared_ptr<crypto::ICrypto>& crypto,
                         const std::shared_ptr<peer::PeerService>& peerService,
                         const std::shared_ptr<ui::ConsoleUI>& consoleUI)
    : config(config), crypto(crypto), peerService(peerService), consoleUI(consoleUI)
{
}

void ChatService::handleIncomingMessage(const std::string& message, std::string& response)
{
    try
    {
        json jData = json::parse(message);

        if (jData["type"] ==
            message::Message::fromMessageTypeToString(message::MessageType::CONNECT))
        {
            message::ConnectionMessage message(jData);
            handleIncomingConnectionMessage(message, response);
        }
        else if (jData["type"] ==
                 message::Message::fromMessageTypeToString(message::MessageType::TEXT_MESSAGE))
        {
            try
            {
                message::TextMessage message(
                    jData, config->get(config::ConfigField::PRIVATE_KEY), crypto);
                handleIncomingMessage(message, response);
            }
            catch (const std::exception& e)
            {
                consoleUI->printLog("[SERVER] error while handleing incoming message: " +
                                    std::string(e.what()));
            }
        }
        else if (jData["type"] ==
                 message::Message::fromMessageTypeToString(message::MessageType::DISCONNECT))
        {
            message::DisconnectionMessage message(jData);
            handleIncomingDisconnectionMessage(message, response);
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

        if (jData["type"] ==
            message::Message::fromMessageTypeToString(message::MessageType::CONNECT_RESPONSE))
        {
            message::ConnectionMessageResponse response(jData);
            handleOutgoingConnectionMessage(response);
        }
        else if (jData["type"] ==
                 message::Message::fromMessageTypeToString(message::MessageType::TEXT_MESSAGE))
        {
            message::TextMessageResponse response(jData);
            handleOutgoingMessage(response);
        }
        else if (jData["type"] == message::Message::fromMessageTypeToString(
                                      message::MessageType::DISCONNECT_RESPONSE))
        {
            message::DisconnectionMessageResponse response(jData);
            handleOutgoingDisconnectionMessage(response);
        }
    }
    catch (json::exception& e)
    {
        consoleUI->printLog("[CLIENT] handle outgoing message error: " + std::string(e.what()));
    }
}
}  // namespace chat