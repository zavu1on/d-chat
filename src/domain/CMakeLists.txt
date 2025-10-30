add_library(
    d-chat_domain
    STATIC
    chat/ChatService.cpp
)

target_include_directories(
    d-chat_domain 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/chat
)