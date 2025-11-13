#pragma once

#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "ConnectionMessage.hpp"
#include "ConsoleUI.hpp"
#include "DisconnectionMessage.hpp"
#include "IConfig.hpp"
#include "ICrypto.hpp"
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
    std::shared_ptr<ui::ConsoleUI> consoleUI;

protected:
    void handleIncomingConnectionMessage(const message::ConnectionMessage& message,
                                         std::string& response);
    void handleOutgoingConnectionMessage(const message::ConnectionMessageResponse& response);
    void handleIncomingMessage(const message::TextMessage& message, std::string& response);
    void handleOutgoingMessage(const message::TextMessageResponse& response);
    void handleIncomingDisconnectionMessage(const message::DisconnectionMessage& message,
                                            std::string& response);
    void handleOutgoingDisconnectionMessage(const message::DisconnectionMessageResponse& response);

public:
    ChatService(const std::shared_ptr<config::IConfig>& config,
                const std::shared_ptr<crypto::ICrypto>& crypto,
                const std::shared_ptr<peer::PeerService>& peerService,
                const std::shared_ptr<ui::ConsoleUI>& consoleUI);

    // server side handle request
    void handleIncomingMessage(const std::string& message, std::string& response);
    // client side handle response
    void handleOutgoingMessage(const std::string& response);
};
}  // namespace chat