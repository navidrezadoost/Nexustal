cmake_minimum_required(VERSION 3.27)
project(nexustal_engine VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt")
    find_package(jwt-cpp CONFIG QUIET)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nexustal_engine
    application/auth_service.cpp
    application/invitation_service.cpp
    application/wizard_service.cpp
    infrastructure/adapters/jwt_token_service.cpp
    infrastructure/adapters/postgres_invitation_repository.cpp
    infrastructure/adapters/postgres_user_repository.cpp
    infrastructure/adapters/redis_notification_service.cpp
    src/main.cpp
    src/controllers/auth_controller.cpp
    src/controllers/invitation_controller.cpp
    src/controllers/sse_controller.cpp
    src/controllers/wizard_controller.cpp
    src/filters/jwt_auth_filter.cpp
    src/platform/migration_runner.cpp
)

target_include_directories(nexustal_engine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nexustal_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(nexustal_engine
    PRIVATE
        Drogon::Drogon
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
)

if (TARGET jwt-cpp::jwt-cpp)
    target_link_libraries(nexustal_engine PRIVATE jwt-cpp::jwt-cpp)
endif()

file(COPY ${CMAKE_SOURCE_DIR}/migrations DESTINATION ${CMAKE_BINARY_DIR})

if (BUILD_TESTING)
    find_package(GTest REQUIRED)

    add_executable(identity_tests
        tests/domain/identity_tests.cpp
    )

    add_executable(wizard_service_tests
        application/wizard_service.cpp
        tests/domain/wizard_service_tests.cpp
    )

    add_executable(auth_service_tests
        application/auth_service.cpp
        tests/domain/auth_service_tests.cpp
    )

    add_executable(invitation_service_tests
        application/invitation_service.cpp
        tests/domain/invitation_service_tests.cpp
    )

    add_executable(capabilities_tests
        tests/domain/capabilities_tests.cpp
    )

    target_include_directories(identity_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(wizard_service_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(auth_service_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(invitation_service_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(capabilities_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_link_libraries(identity_tests
        PRIVATE
            GTest::gtest_main
    )

    target_link_libraries(wizard_service_tests
        PRIVATE
            GTest::gtest_main
    )

    target_link_libraries(auth_service_tests
        PRIVATE
            GTest::gtest_main
    )

    target_link_libraries(invitation_service_tests
        PRIVATE
            GTest::gtest_main
    )

    target_link_libraries(capabilities_tests
        PRIVATE
            GTest::gtest_main
    )

    set_target_properties(identity_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    set_target_properties(wizard_service_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    set_target_properties(auth_service_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    set_target_properties(invitation_service_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    set_target_properties(capabilities_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME IdentityTests COMMAND ${CMAKE_BINARY_DIR}/tests/identity_tests)
    add_test(NAME WizardServiceTests COMMAND ${CMAKE_BINARY_DIR}/tests/wizard_service_tests)
    add_test(NAME AuthServiceTests COMMAND ${CMAKE_BINARY_DIR}/tests/auth_service_tests)
    add_test(NAME InvitationServiceTests COMMAND ${CMAKE_BINARY_DIR}/tests/invitation_service_tests)
    add_test(NAME CapabilitiesTests COMMAND ${CMAKE_BINARY_DIR}/tests/capabilities_tests)
endif()
