#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "BlockRangeMessage.hpp"
#include "BlockchainErrorMessage.hpp"
#include "BlockchainService.hpp"
#include "ConnectionMessage.hpp"
#include "ConsoleUI.hpp"
#include "DisconnectionMessage.hpp"
#include "ErrorMessage.hpp"
#include "IConfig.hpp"
#include "ICrypto.hpp"
#include "MessageService.hpp"
#include "PeerListMessage.hpp"
#include "PeerService.hpp"
#include "TextMessage.hpp"

namespace chat
{
using json = nlohmann::json;

class ChatService
{
private:
    std::shared_ptr<config::IConfig> config;
    std::shared_ptr<crypto::ICrypto> crypto;
    std::shared_ptr<peer::PeerService> peerService;
    std::shared_ptr<blockchain::BlockchainService> blockchainService;
    std::shared_ptr<message::MessageService> messageService;
    std::shared_ptr<ui::ConsoleUI> consoleUI;

protected:
    void handleIncomingErrorMessage(const json& jData,
                                    const std::string& error,
                                    std::string& response);
    void handleOutgoingBlockchainErrorMessage(
        const message::BlockchainErrorMessageResponse& response);
    void handleOutgoingErrorMessage(const message::ErrorMessageResponse& response);
    void handleIncomingConnectionMessage(const message::ConnectionMessage& message,
                                         std::string& response);
    void handleOutgoingConnectionMessage(const message::ConnectionMessageResponse& response);
    void handleIncomingTextMessage(const message::TextMessage& message,
                                   const std::string& messageDump,
                                   std::string& response);
    void handleOutgoingTextMessage(const message::TextMessageResponse& response);
    void handleIncomingPeerListMessage(const message::PeerListMessage& message,
                                       std::string& response);
    void handleOutgoingPeerListMessage(const message::PeerListMessageResponse& response);
    void handleIncomingDisconnectionMessage(const message::DisconnectionMessage& message,
                                            std::string& response);
    void handleIncomingBlockRangeMessage(const message::BlockRangeMessage& message,
                                         std::string& response);
    void handleOutgoingBlockRangeMessage(const message::BlockRangeMessageResponse& response);
    void handleOutgoingDisconnectionMessage(const message::DisconnectionMessageResponse& response);

public:
    ChatService(const std::shared_ptr<config::IConfig>& config,
                const std::shared_ptr<crypto::ICrypto>& crypto,
                const std::shared_ptr<peer::PeerService>& peerService,
                const std::shared_ptr<blockchain::BlockchainService>& blockchainService,
                const std::shared_ptr<message::MessageService>& messageService,
                const std::shared_ptr<ui::ConsoleUI>& consoleUI);

    // server side handle request
    void handleIncomingMessage(const json& jMessage, std::string& response);
    // client side handle response
    void handleOutgoingMessage(const std::string& response);
};
}  // namespace chat