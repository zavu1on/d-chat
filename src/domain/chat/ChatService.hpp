#pragma once

#include <memory>
#include <string>

#include "ConnectMessage.hpp"
#include "ConsoleUI.hpp"
#include "DisconnectMessage.hpp"
#include "IConfig.hpp"
#include "ICrypto.hpp"
#include "PeerService.hpp"
#include "SendMessage.hpp"

class ChatService
{
private:
    std::shared_ptr<IConfig> config;
    std::shared_ptr<ICrypto> crypto;
    std::shared_ptr<PeerService> peerService;
    std::shared_ptr<ConsoleUI> consoleUI;

protected:
    void handleIncomingConnectMessage(const ConnectMessage& message, std::string& response);
    void handleOutgoingConnectMessage(const ConnectResponseMessage& response);
    void handleIncomingMessage(const SendMessage& message, std::string& response);
    void handleOutgoingMessage(const SendResponseMessage& response);
    void handleIncomingDisconnectMessage(const DisconnectMessage& message, std::string& response);
    void handleOutgoingDisconnectMessage(const DisconnectResponseMessage& response);

public:
    ChatService(const std::shared_ptr<IConfig>& config,
                const std::shared_ptr<ICrypto>& crypto,
                const std::shared_ptr<PeerService>& peerService,
                const std::shared_ptr<ConsoleUI>& consoleUI);

    // server side handle request
    void handleIncomingMessage(const std::string& message, std::string& response);
    // client side handle response
    void handleOutgoingMessage(const std::string& response);
};
