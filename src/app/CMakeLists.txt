add_executable(
    d-chat_main
    main.cpp
    ChatApplication.cpp
)

target_link_libraries(d-chat_main
    PRIVATE
    d-chat_ui
    d-chat_infra
    d-chat_service
    d-chat_domain
)

target_include_directories(
    d-chat_main
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
