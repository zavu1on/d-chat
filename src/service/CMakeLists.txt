add_library(
    d-chat_service
    STATIC
    network/SocketServer.cpp
    network/SocketClient.cpp
    json/JsonFile.hpp
)

set(nlohmann_json_DIR "${CMAKE_SOURCE_DIR}/vcpkg_installed/x64-windows/share/nlohmann_json")
find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(
    d-chat_service
    PUBLIC
    nlohmann_json::nlohmann_json
)


target_link_libraries(
    d-chat_service
    PRIVATE
    ws2_32
)

target_include_directories(
    d-chat_service 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/json
)