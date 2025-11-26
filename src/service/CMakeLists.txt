add_library(
    d-chat_service
    STATIC
    network/SocketServer.cpp
    network/SocketClient.cpp
    json/JsonFile.hpp
    crypto/OpenSSLCrypto.cpp
    db/DBFile.cpp
)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)
find_package(SQLite3 REQUIRED)

target_link_libraries(
    d-chat_service
    PUBLIC
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)

target_link_libraries(
    d-chat_service
    PRIVATE
    ws2_32
    OpenSSL::Crypto
    OpenSSL::SSL
    d-chat_utils
)

target_include_directories(
    d-chat_service
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/json
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/db
)

set(DB_FILE "${CMAKE_SOURCE_DIR}/d-chat.db")

add_custom_command(
    TARGET d-chat_service POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DB_FILE}"
        "${CMAKE_BINARY_DIR}/d-chat.db"
)