add_executable(d-chat_main main.cpp)

target_link_libraries(
    d-chat_main
    PRIVATE
    d-chat_infra
)