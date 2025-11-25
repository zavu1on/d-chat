#include "ChatService.hpp"

#include "Block.hpp"
#include "GlobalState.hpp"
#include "timestamp.hpp"
#include "uuid.hpp"

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

        message::ErrorMessageResponse errorMessage(utils::uuidv4(), me, to, timestamp, error);
        json jData;
        errorMessage.serialize(jData);

        response = jData.dump();
    }
    else
        response = "{}";
}

void ChatService::handleOutgoingBlockchainErrorMessage(
    const message::BlockchainErrorMessageResponse& response)
{
    peer::UserPeer from = response.getFrom();
    const message::BlockchainErrorMessageResponsePayload& payload = response.getPayload();

    consoleUI->printLog("[CLIENT] Blockchain error response from:  " + from.host + ":" +
                        std::to_string(from.port) + " " + payload.error + "\n");

    if (messageService->removeMessageByBlockHash(payload.block.hash))
    {
        consoleUI->printLog(
            "[CLIENT] can not save message without successfully delivered blocks, removed message "
            "for block " +
            payload.block.hash + "\n");
    }
}

void ChatService::handleOutgoingErrorMessage(const message::ErrorMessageResponse& response)
{
    peer::UserPeer from = response.getFrom();
    const message::ErrorMessageResponsePayload& payload = response.getPayload();

    consoleUI->printLog("[CLIENT] Error response from: " + from.host + ":" +
                        std::to_string(from.port) + " " + payload.error + "\n");
}

void ChatService::handleIncomingConnectionMessage(const message::ConnectionMessage& message,
                                                  std::string& response)
{
    peer::UserPeer from = message.getFrom();
    peer::UserPeer to = message.getTo();
    const message::ConnectionMessagePayload& payload = message.getPayload();

    peerService->addPeer({ from.host, from.port, from.publicKey });

    peer::UserPeer me{ to.host, to.port, config->get(config::ConfigField::PUBLIC_KEY) };
    uint64_t timestamp = utils::getTimestamp();

    unsigned int peersToReceive = peerService->getPeersCount();
    unsigned int missingCount = blockchainService->countBlocksAfterHash(payload.lastBlockHash);

    message::ConnectionMessageResponse responseMessage(
        utils::uuidv4(), me, from, timestamp, peersToReceive, missingCount);

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

    const message::ConnectionMessageResponsePayload& payload = response.getPayload();
    config::GlobalState& globalState = config::GlobalState::getInstance();

    globalState.setPeersToReceive(payload.peersToReceive);
    globalState.setMissingBlocksCount(payload.missingBlocksCount);
}

void ChatService::handleIncomingTextMessage(const message::TextMessage& message,
                                            const std::string& messageDump,
                                            std::string& response)
{
    peer::UserPeer from = message.getFrom();
    peer::UserPeer to = message.getTo();
    const message::TextMessagePayload& payload = message.getPayload();

    messageService->insertSecretMessage(message, messageDump, message.getBlockHash());
    peerService->addChatPeer(from);

    consoleUI->printLog("[SERVER] received message from " + from.host + ":" +
                        std::to_string(from.port) + ": " + payload.message + "\n");

    peer::UserPeer me{ to.host, to.port, config->get(config::ConfigField::PUBLIC_KEY) };
    uint64_t timestamp = utils::getTimestamp();

    message::TextMessageResponse responseMessage(utils::uuidv4(), me, from, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
}

void ChatService::handleOutgoingTextMessage(const message::TextMessageResponse&) {}

void ChatService::handleIncomingPeerListMessage(const message::PeerListMessage& message,
                                                std::string& response)
{
    peer::UserPeer from = message.getFrom();
    peer::UserPeer to = message.getTo();
    const message::PeerListMessagePayload& payload = message.getPayload();
    std::vector<peer::UserPeer> peers;

    for (size_t i = payload.start; i < payload.start + payload.count; i++)
    {
        try
        {
            peer::UserPeer peer = peerService->getPeer(i);
            if (peer == from) continue;

            peers.push_back(peer);
        }
        catch (const std::out_of_range&)
        {
            break;
        }
    }

    message::PeerListMessageResponse responseMessage(
        utils::uuidv4(), to, from, utils::getTimestamp(), peers);

    json jData;
    responseMessage.serialize(jData);
    response = jData.dump();
}

void ChatService::handleOutgoingPeerListMessage(const message::PeerListMessageResponse& response)
{
    const message::PeerListMessageResponsePayload& payload = response.getPayload();

    for (const auto& peer : payload.peers)
    {
        peerService->addPeer(peer);
        consoleUI->printLog("[CLIENT] added peer from PEER_LIST " + peer.host + ":" +
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
    message::DisconnectionMessageResponse responseMessage(utils::uuidv4(), to, from, timestamp);

    json jData;
    responseMessage.serialize(jData);

    response = jData.dump();
    consoleUI->printLog("[SERVER] removed peer " + from.host + ":" + std::to_string(from.port) +
                        "\n");
}

void ChatService::handleIncomingBlockRangeMessage(const message::BlockRangeMessage& message,
                                                  std::string& response)
{
    const message::BlockRangeMessagePayload& payload = message.getPayload();
    peer::UserPeer from = message.getFrom();
    peer::UserPeer to = message.getTo();

    std::vector<blockchain::Block> blocks;
    blockchainService->getBlocksByIndexRange(
        payload.start, payload.count, payload.lastHash, blocks);

    message::BlockRangeMessageResponse responseMessage(
        utils::uuidv4(), to, from, utils::getTimestamp(), blocks);
    json jData;
    responseMessage.serialize(jData);
    response = jData.dump();
}

void ChatService::handleOutgoingBlockRangeMessage(
    const message::BlockRangeMessageResponse& response)
{
    const message::BlockRangeMessageResponsePayload& payload = response.getPayload();

    blockchainService->addNewBlockRange(payload.blocks);
    consoleUI->printLog("[CLIENT] received " + std::to_string(payload.blocks.size()) +
                        " blocks from " + response.getFrom().host + ":" +
                        std::to_string(response.getFrom().port) + "\n");
}

void ChatService::handleOutgoingDisconnectionMessage(const message::DisconnectionMessageResponse&)
{
}

ChatService::ChatService(const std::shared_ptr<config::IConfig>& config,
                         const std::shared_ptr<crypto::ICrypto>& crypto,
                         const std::shared_ptr<peer::PeerService>& peerService,
                         const std::shared_ptr<blockchain::BlockchainService>& blockchainService,
                         const std::shared_ptr<message::MessageService>& messageService,
                         const std::shared_ptr<ui::ConsoleUI>& consoleUI)
    : config(config),
      crypto(crypto),
      peerService(peerService),
      blockchainService(blockchainService),
      messageService(messageService),
      consoleUI(consoleUI)
{
}

void ChatService::handleIncomingMessage(const json& jMessage, std::string& response)
{
    try
    {
        if (jMessage["type"] ==
            message::Message::fromMessageTypeToString(message::MessageType::CONNECT))
        {
            message::ConnectionMessage message(jMessage);
            handleIncomingConnectionMessage(message, response);
        }
        else if (jMessage["type"] ==
                 message::Message::fromMessageTypeToString(message::MessageType::TEXT_MESSAGE))
        {
            message::TextMessage message(
                jMessage, config->get(config::ConfigField::PRIVATE_KEY), crypto);
            handleIncomingTextMessage(message, jMessage.dump(), response);
        }
        else if (jMessage["type"] ==
                 message::Message::fromMessageTypeToString(message::MessageType::PEER_LIST))
        {
            message::PeerListMessage message(jMessage);
            handleIncomingPeerListMessage(message, response);
        }
        else if (jMessage["type"] == message::Message::fromMessageTypeToString(
                                         message::MessageType::BLOCK_RANGE_REQUEST))
        {
            message::BlockRangeMessage message(jMessage);
            handleIncomingBlockRangeMessage(message, response);
        }
        else if (jMessage["type"] ==
                 message::Message::fromMessageTypeToString(message::MessageType::DISCONNECT))
        {
            message::DisconnectionMessage message(jMessage);
            handleIncomingDisconnectionMessage(message, response);
        }
        else
            response = "{}";
    }
    catch (std::exception& error)
    {
        handleIncomingErrorMessage(jMessage, std::string(error.what()), response);
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
        if (jData["type"] == message::Message::fromMessageTypeToString(
                                 message::MessageType::BLOCKCHAIN_ERROR_RESPONSE))
        {
            message::BlockchainErrorMessageResponse response(jData);
            handleOutgoingBlockchainErrorMessage(response);
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
            handleOutgoingTextMessage(response);
        }
        else if (jData["type"] == message::Message::fromMessageTypeToString(
                                      message::MessageType::PEER_LIST_RESPONSE))
        {
            message::PeerListMessageResponse response(jData);
            handleOutgoingPeerListMessage(response);
        }
        else if (jData["type"] == message::Message::fromMessageTypeToString(
                                      message::MessageType::BLOCK_RANGE_RESPONSE))
        {
            message::BlockRangeMessageResponse response(jData);
            handleOutgoingBlockRangeMessage(response);
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
        consoleUI->printLog("[CLIENT] handle outgoing message error: " + std::string(error.what()) +
                            "\n");
    }
}
}  // namespace chat