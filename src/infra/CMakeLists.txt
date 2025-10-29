add_library(
    d-chat_infra
    STATIC
    network/SocketServer.cpp
    network/SocketClient.cpp
)

target_link_libraries(d-chat_infra PRIVATE ws2_32)
target_include_directories( # link .hpp files 
    d-chat_infra 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/network
)