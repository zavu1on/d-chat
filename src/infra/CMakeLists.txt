add_library(
    d-chat_infra
    STATIC
    network/TCPServer.cpp
    network/TCPClient.cpp
    config/JsonConfig.cpp
    message/ConnectionMessage.cpp
    message/TextMessage.cpp
    message/DisconnectionMessage.cpp
    message/ErrorMessage.cpp
    message/PeerListMessage.cpp
    message/BlockRangeMessage.cpp
    message/BlockchainErrorMessage.cpp
    message/MessageDB.cpp
    blockchain/ChainDB.cpp
    peer/PeerDB.cpp
)

target_link_libraries(
    d-chat_infra 
    PRIVATE
    d-chat_utils
    d-chat_service 
    d-chat_domain
)

target_include_directories(
    d-chat_infra 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/message
    ${CMAKE_CURRENT_SOURCE_DIR}/blockchain
    ${CMAKE_CURRENT_SOURCE_DIR}/peer
)

set(CONFIG_FILE "${CMAKE_SOURCE_DIR}/d-chat_config.json")

add_custom_command(
    TARGET d-chat_infra POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CONFIG_FILE}"
        "${CMAKE_BINARY_DIR}/d-chat_config.json"
)
