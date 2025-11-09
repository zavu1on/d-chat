add_library(
    d-chat_ui
    STATIC
    cli/ConsoleUI.cpp
)

target_include_directories(
    d-chat_ui
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/cli
)