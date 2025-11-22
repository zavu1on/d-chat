add_library(
    d-chat_domain
    STATIC
    chat/ChatService.cpp
    config/IConfig.cpp
    config/GlobalState.cpp
    message/Message.cpp
    peer/UserPeer.cpp
    peer/PeerService.cpp
    blockchain/Block.cpp
    blockchain/BlockchainService.cpp
    message/MessageService.cpp
)

target_include_directories(
    d-chat_domain
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/chat
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/message
    ${CMAKE_CURRENT_SOURCE_DIR}/peer
    ${CMAKE_CURRENT_SOURCE_DIR}/blockchain
)

set(nlohmann_json_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share/nlohmann_json")
find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(
    d-chat_domain
    PUBLIC
    d-chat_ui
    nlohmann_json::nlohmann_json
)

target_link_libraries(
    d-chat_domain
    PRIVATE
    d-chat_utils
    d-chat_service
    d-chat_infra
)