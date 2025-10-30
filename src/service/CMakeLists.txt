add_library(
    d-chat_service
    STATIC
    network/SocketServer.cpp
    network/SocketClient.cpp
)

target_link_libraries(d-chat_service PRIVATE ws2_32)

target_include_directories(
    d-chat_service 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/network
)