#include "ChatService.hpp"

#include "GlobalState.hpp"
#include "timestamp.hpp"

namespace chat
{
void ChatService::handleIncomingErrorMessage(const json& jData,
                                             const std::string& error,
                                             std::string& response)
{
    if (jData.contains("from") && jData["from"].contains("host") &&
        jData["from"].contains("port") && jData["from"].contains("public_key"))
    {
        std::string host = config->get(config::ConfigField::HOST);
        unsigned short port =
            static_cast<unsigned short>(stoi(config->get(config::ConfigField::PORT)));
        std::string publicKey = config->get(config::ConfigField::PUBLIC_KEY);

        peer::UserPeer me(host, port, publicKey);
        peer::UserPeer to(jData["from"]);
        uint64_t timestamp = utils::getTimestamp();

        message::ErrorMessageResponse errorMessage(me, to, timestamp, error);
        json jData;
        errorMessage.serialize(jData);

        response = jData.dump();
    }
    else
        response = "";
}

void ChatService::handleOutgoingErrorMessage(const message::ErrorMessageResponse& response)
{
    peer::UserPeer from = response.getFrom();
    message::ErrorMessageResponsePayload payload = response.getPayload();

    consoleUI->printLog("[CLIENT] Error response from:  " + from.host + ":" +
                        std::to_string(from.port) + " " + payload.error);
}

void ChatService::handleIncomingConnectionMessage(const message::ConnectionMessage& message,
                                                  std::string& response)
{
    peer::UserPeer from = message.getFrom();
    peer::UserPeer to = message.getTo();

    peerService->addPeer({ from.host, from.port, from.publicKey });

    peer::UserPeer me{ to.host, to.port, config->get(config::ConfigField::PUBLIC_KEY) };
    uint64_t timestamp = utils::getTimestamp();

    unsigned int peersToReceive = peerService->getPeersCount();
    message::ConnectionMessageResponse responseMessage(me, from, timestamp, peersToReceive);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
    consoleUI->printLog("[SERVER] accepted new peer from " + from.host + ":" +
                        std::to_string(from.port) + "\n");
}

void ChatService::handleOutgoingConnectionMessage(
    const message::ConnectionMessageResponse& response)
{
    peer::UserPeer from = response.getFrom();
    peerService->addPeer({ from.host, from.port, from.publicKey });

    message::ConnectionMessageResponsePayload payload = response.getPayload();
    config::GlobalState& globalState = config::GlobalState::getInstance();
    globalState.setPeersToReceive(payload.peersToReceive);
}

void ChatService::handleIncomingMessage(const message::TextMessage& message, std::string& response)
{
    peer::UserPeer from = message.getFrom();
    peer::UserPeer to = message.getTo();
    message::TextMessagePayload payload = message.getPayload();

    consoleUI->printLog("[SERVER] received message from " + from.host + ":" +
                        std::to_string(from.port) + ": " + payload.message + "\n");

    peer::UserPeer me{ to.host, to.port, config->get(config::ConfigField::PUBLIC_KEY) };
    uint64_t timestamp = utils::getTimestamp();

    message::TextMessageResponse responseMessage(me, from, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
}

void ChatService::handleOutgoingMessage(const message::TextMessageResponse&) {}

void ChatService::handleIncomingPeerListMessage(const message::PeerListMessage& message,
                                                std::string& response)
{
    peer::UserPeer from = message.getFrom();
    peer::UserPeer to = message.getTo();
    message::PeerListMessagePayload payload = message.getPayload();
    std::vector<peer::UserPeer> peers;

    for (size_t i = payload.start; i < payload.end; i++)
    {
        try
        {
            peer::UserPeer peer = peerService->getPeer(i);
            if (peer == from) continue;

            peers.push_back(peer);
        }
        catch (const std::range_error&)
        {
            break;
        }
    }

    message::PeerListMessageResponse responseMessage(to, from, utils::getTimestamp(), peers);

    json jData;
    responseMessage.serialize(jData);
    response = jData.dump();
}

void ChatService::handleOutgoingPeerListMessage(const message::PeerListMessageResponse& response)
{
    message::PeerListMessageResponsePayload payload = response.getPayload();

    for (const auto& peer : payload.peers)
    {
        peerService->addPeer(peer);
        consoleUI->printLog("[CLIENT] added peer from PEER_LIST" + peer.host + ":" +
                            std::to_string(peer.port) + "\n");
    }
}

void ChatService::handleIncomingDisconnectionMessage(const message::DisconnectionMessage& message,
                                                     std::string& response)
{
    peer::UserPeer from = message.getFrom();
    peer::UserPeer to = message.getTo();

    peerService->removePeer(from);

    uint64_t timestamp = utils::getTimestamp();
    message::DisconnectionMessageResponse responseMessage(to, from, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
    consoleUI->printLog("[SERVER] removed peer " + from.host + ":" + std::to_string(from.port) +
                        "\n");
}

void ChatService::handleOutgoingDisconnectionMessage(const message::DisconnectionMessageResponse&)
{
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
            message::TextMessage message(
                jData, config->get(config::ConfigField::PRIVATE_KEY), crypto);
            handleIncomingMessage(message, response);
        }
        else if (jData["type"] ==
                 message::Message::fromMessageTypeToString(message::MessageType::PEER_LIST))
        {
            message::PeerListMessage message(jData);
            handleIncomingPeerListMessage(message, response);
        }
        else if (jData["type"] ==
                 message::Message::fromMessageTypeToString(message::MessageType::DISCONNECT))
        {
            message::DisconnectionMessage message(jData);
            handleIncomingDisconnectionMessage(message, response);
        }
    }
    catch (std::exception& error)
    {
        try
        {
            json jData = json::parse(message);
            handleIncomingErrorMessage(jData, std::string(error.what()), response);
        }
        catch (const std::exception& unhandledError)
        {
            consoleUI->printLog("[SERVER] handle incoming message error: " +
                                std::string(unhandledError.what()));
        }
    }
}

void ChatService::handleOutgoingMessage(const std::string& response)
{
    try
    {
        json jData = json::parse(response);

        if (jData["type"] ==
            message::Message::fromMessageTypeToString(message::MessageType::ERROR_RESPONSE))
        {
            message::ErrorMessageResponse response(jData);
            handleOutgoingErrorMessage(response);
        }
        if (jData["type"] ==
            message::Message::fromMessageTypeToString(message::MessageType::CONNECT_RESPONSE))
        {
            message::ConnectionMessageResponse response(jData);
            handleOutgoingConnectionMessage(response);
        }
        else if (jData["type"] == message::Message::fromMessageTypeToString(
                                      message::MessageType::TEXT_MESSAGE_RESPONSE))
        {
            message::TextMessageResponse response(jData);
            handleOutgoingMessage(response);
        }
        else if (jData["type"] == message::Message::fromMessageTypeToString(
                                      message::MessageType::PEER_LIST_RESPONSE))
        {
            message::PeerListMessageResponse response(jData);
            handleOutgoingPeerListMessage(response);
        }

        else if (jData["type"] == message::Message::fromMessageTypeToString(
                                      message::MessageType::DISCONNECT_RESPONSE))
        {
            message::DisconnectionMessageResponse response(jData);
            handleOutgoingDisconnectionMessage(response);
        }
    }
    catch (std::exception& error)
    {
        consoleUI->printLog("[CLIENT] handle outgoing message error: " + std::string(error.what()));
    }
}
}  // namespace chat