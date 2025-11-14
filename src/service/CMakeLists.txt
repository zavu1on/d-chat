add_library(
    d-chat_service
    STATIC
    network/SocketServer.cpp
    network/SocketClient.cpp
    json/JsonFile.hpp
    crypto/XORCrypto.cpp
    crypto/OpenSSLCrypto.cpp
    blockchain/Block.cpp
    blockchain/Blockchain.cpp
)


# set(nlohmann_json_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-mingw-static/share/nlohmann_json")
# set(OPENSSL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-mingw-static/share/openssl")
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)

target_link_libraries(
    d-chat_service
    PUBLIC
    nlohmann_json::nlohmann_json
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
    ${CMAKE_CURRENT_SOURCE_DIR}/blockchain
)