find_package(GTest CONFIG REQUIRED)

add_library(
    test_helpers
    STATIC
    helpers/test_helpers.cpp
    helpers/XORCrypto.cpp
)

target_include_directories(
    test_helpers
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers
)

target_link_libraries(
    test_helpers
    PUBLIC
    d-chat_domain
    d-chat_service
    d-chat_infra
    d-chat_utils
    ws2_32
)

# Unit tests
add_executable(
    unit_tests
    unit/crypto_test.cpp
    unit/block_test.cpp
    unit/message_test.cpp
    unit/config_test.cpp
    unit/utils_test.cpp
    unit/database_test.cpp
    unit/xor_crypto_test.cpp
)

target_link_libraries(
    unit_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_helpers
    d-chat_domain
    d-chat_service
    d-chat_infra
    d-chat_ui
)

# Integration tests
add_executable(
    integration_tests
    integration/blockchain_service_test.cpp
    integration/message_service_test.cpp
    integration/peer_service_test.cpp
    integration/chat_service_test.cpp
    integration/network_test.cpp
    integration/xor_crypto_test.cpp
)

target_link_libraries(
    integration_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_helpers
    d-chat_domain
    d-chat_service
    d-chat_infra
    d-chat_ui
)

# E2E tests
add_executable(
    e2e_tests
    e2e/full_chat_scenario_test.cpp
    e2e/blockchain_sync_test.cpp
)

target_link_libraries(
    e2e_tests
    PRIVATE
    GTest::gtest
    GTest::gtest_main
    test_helpers
    # d-chart_app
    d-chat_domain
    d-chat_service
    d-chat_infra
    d-chat_ui
)

enable_testing()
add_test(NAME UnitTests COMMAND unit_tests)
add_test(NAME IntegrationTests COMMAND integration_tests)
add_test(NAME E2ETests COMMAND e2e_tests)