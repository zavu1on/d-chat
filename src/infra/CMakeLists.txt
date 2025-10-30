add_library(
    d-chat_infra
    STATIC
    chat/TCPServer.cpp
    chat/TCPClient.cpp
)

target_link_libraries(
    d-chat_infra 
    PRIVATE 
    d-chat_service 
    d-chat_domain
)

target_include_directories(
    d-chat_infra 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/chat
)
