cmake_minimum_required(VERSION 3.27)
project(nexustal_engine VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

option(ENABLE_QUICKJS "Enable QuickJS scripting sandbox" ON)
set(QUICKJS_PROVIDER "AUTO" CACHE STRING "QuickJS provider: AUTO, CONAN, SUBMODULE, SYSTEM, STUB")
set_property(CACHE QUICKJS_PROVIDER PROPERTY STRINGS AUTO CONAN SUBMODULE SYSTEM STUB)
set(QUICKJS_SUBMODULE_DIR "${CMAKE_SOURCE_DIR}/../third_party/quickjs" CACHE PATH "Path to a vendored QuickJS checkout")

set(NEXUSTAL_QUICKJS_TARGET "")
set(NEXUSTAL_QUICKJS_INCLUDE_DIRS "")

function(nexustal_try_conan_quickjs)
    if (NOT TARGET quickjs::quickjs AND NOT TARGET QuickJS::QuickJS AND NOT TARGET quickjs)
        find_package(quickjs CONFIG QUIET)
        find_package(QuickJS CONFIG QUIET)
    endif()

    if (TARGET quickjs::quickjs)
        set(NEXUSTAL_QUICKJS_TARGET quickjs::quickjs PARENT_SCOPE)
    elseif (TARGET QuickJS::QuickJS)
        set(NEXUSTAL_QUICKJS_TARGET QuickJS::QuickJS PARENT_SCOPE)
    elseif (TARGET quickjs)
        set(NEXUSTAL_QUICKJS_TARGET quickjs PARENT_SCOPE)
    endif()
endfunction()

function(nexustal_try_submodule_quickjs)
    if (TARGET nexustal_quickjs OR TARGET quickjs)
        if (TARGET nexustal_quickjs)
            set(NEXUSTAL_QUICKJS_TARGET nexustal_quickjs PARENT_SCOPE)
        else()
            set(NEXUSTAL_QUICKJS_TARGET quickjs PARENT_SCOPE)
        endif()
        return()
    endif()

    if (EXISTS "${QUICKJS_SUBMODULE_DIR}/quickjs.c" AND EXISTS "${QUICKJS_SUBMODULE_DIR}/libregexp.c")
        add_library(nexustal_quickjs STATIC
            ${QUICKJS_SUBMODULE_DIR}/quickjs.c
            ${QUICKJS_SUBMODULE_DIR}/libregexp.c
            ${QUICKJS_SUBMODULE_DIR}/libunicode.c
            ${QUICKJS_SUBMODULE_DIR}/cutils.c
        )
        target_include_directories(nexustal_quickjs PUBLIC ${QUICKJS_SUBMODULE_DIR})
        target_compile_definitions(nexustal_quickjs PRIVATE CONFIG_VERSION=\"local\")
        if (UNIX)
            target_link_libraries(nexustal_quickjs PUBLIC m dl pthread)
        endif()
        set_target_properties(nexustal_quickjs PROPERTIES POSITION_INDEPENDENT_CODE ON)
        set(NEXUSTAL_QUICKJS_TARGET nexustal_quickjs PARENT_SCOPE)
    endif()
endfunction()

function(nexustal_try_system_quickjs)
    if (TARGET nexustal_quickjs_system)
        set(NEXUSTAL_QUICKJS_TARGET nexustal_quickjs_system PARENT_SCOPE)
        return()
    endif()

    find_path(QUICKJS_INCLUDE_DIR quickjs.h)
    find_library(QUICKJS_LIBRARY NAMES quickjs libquickjs)
    if (QUICKJS_INCLUDE_DIR AND QUICKJS_LIBRARY)
        add_library(nexustal_quickjs_system UNKNOWN IMPORTED)
        set_target_properties(nexustal_quickjs_system PROPERTIES
            IMPORTED_LOCATION "${QUICKJS_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${QUICKJS_INCLUDE_DIR}"
        )
        set(NEXUSTAL_QUICKJS_TARGET nexustal_quickjs_system PARENT_SCOPE)
    endif()
endfunction()

if (ENABLE_QUICKJS)
    if (QUICKJS_PROVIDER STREQUAL "AUTO" OR QUICKJS_PROVIDER STREQUAL "CONAN")
        nexustal_try_conan_quickjs()
    endif()

    if (NOT NEXUSTAL_QUICKJS_TARGET AND (QUICKJS_PROVIDER STREQUAL "AUTO" OR QUICKJS_PROVIDER STREQUAL "SUBMODULE"))
        nexustal_try_submodule_quickjs()
    endif()

    if (NOT NEXUSTAL_QUICKJS_TARGET AND (QUICKJS_PROVIDER STREQUAL "AUTO" OR QUICKJS_PROVIDER STREQUAL "SYSTEM"))
        nexustal_try_system_quickjs()
    endif()
endif()

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/conanfile.txt")
    find_package(jwt-cpp CONFIG QUIET)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(nexustal_engine
    application/auth_service.cpp
    application/execution/request_executor.cpp
    application/endpoint_service.cpp
    application/invitation_service.cpp
    application/role_service.cpp
    application/wizard_service.cpp
    infrastructure/adapters/environment_cache.cpp
    infrastructure/adapters/jwt_token_service.cpp
    infrastructure/adapters/postgres_environment_repository.cpp
    infrastructure/adapters/postgres_endpoint_repository.cpp
    infrastructure/adapters/postgres_invitation_repository.cpp
    infrastructure/adapters/postgres_project_repository.cpp
    infrastructure/adapters/postgres_team_repository.cpp
    infrastructure/adapters/postgres_user_repository.cpp
    infrastructure/adapters/redis_notification_service.cpp
    infrastructure/adapters/sse_connection_manager.cpp
    infrastructure/sandbox/nt_api_bindings.cpp
    infrastructure/sandbox/quickjs_sandbox.cpp
    src/main.cpp
    src/controllers/auth_controller.cpp
    src/controllers/endpoint_controller.cpp
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

if (ENABLE_QUICKJS AND NEXUSTAL_QUICKJS_TARGET)
    target_compile_definitions(nexustal_engine PRIVATE NEXUSTAL_HAS_QUICKJS=1)
    target_link_libraries(nexustal_engine PRIVATE ${NEXUSTAL_QUICKJS_TARGET})
else()
    message(WARNING "QuickJS not found. Scripting sandbox will be disabled.")
    target_compile_definitions(nexustal_engine PRIVATE NEXUSTAL_HAS_QUICKJS=0)
endif()

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

    add_executable(role_service_tests
        application/role_service.cpp
        tests/domain/role_service_tests.cpp
    )

    add_executable(endpoint_service_tests
        application/endpoint_service.cpp
        application/role_service.cpp
        tests/domain/endpoint_service_tests.cpp
    )

    add_executable(dynamic_variables_tests
        tests/execution/dynamic_variables_tests.cpp
    )

    add_executable(request_executor_tests
        application/execution/request_executor.cpp
        infrastructure/adapters/environment_cache.cpp
        infrastructure/sandbox/nt_api_bindings.cpp
        infrastructure/sandbox/quickjs_sandbox.cpp
        tests/execution/request_executor_tests.cpp
    )

    add_executable(quickjs_sandbox_tests
        infrastructure/sandbox/nt_api_bindings.cpp
        infrastructure/sandbox/quickjs_sandbox.cpp
        tests/sandbox/quickjs_sandbox_tests.cpp
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

    target_include_directories(role_service_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(endpoint_service_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(dynamic_variables_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(request_executor_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

    target_include_directories(quickjs_sandbox_tests
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

    target_link_libraries(role_service_tests
        PRIVATE
            Drogon::Drogon
            GTest::gtest_main
    )

    target_link_libraries(endpoint_service_tests
        PRIVATE
            Drogon::Drogon
            GTest::gtest_main
    )

    target_link_libraries(dynamic_variables_tests
        PRIVATE
            nlohmann_json::nlohmann_json
            GTest::gtest_main
    )

    target_link_libraries(request_executor_tests
        PRIVATE
            Drogon::Drogon
            nlohmann_json::nlohmann_json
            GTest::gtest_main
    )

    target_link_libraries(quickjs_sandbox_tests
        PRIVATE
            nlohmann_json::nlohmann_json
            GTest::gtest_main
    )

    if (ENABLE_QUICKJS AND NEXUSTAL_QUICKJS_TARGET)
        target_compile_definitions(request_executor_tests PRIVATE NEXUSTAL_HAS_QUICKJS=1)
        target_link_libraries(request_executor_tests PRIVATE ${NEXUSTAL_QUICKJS_TARGET})
        target_compile_definitions(quickjs_sandbox_tests PRIVATE NEXUSTAL_HAS_QUICKJS=1)
        target_link_libraries(quickjs_sandbox_tests PRIVATE ${NEXUSTAL_QUICKJS_TARGET})
    else()
        target_compile_definitions(request_executor_tests PRIVATE NEXUSTAL_HAS_QUICKJS=0)
        target_compile_definitions(quickjs_sandbox_tests PRIVATE NEXUSTAL_HAS_QUICKJS=0)
    endif()

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

    set_target_properties(role_service_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    set_target_properties(endpoint_service_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    set_target_properties(dynamic_variables_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    set_target_properties(request_executor_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    set_target_properties(quickjs_sandbox_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_test(NAME IdentityTests COMMAND ${CMAKE_BINARY_DIR}/tests/identity_tests)
    add_test(NAME WizardServiceTests COMMAND ${CMAKE_BINARY_DIR}/tests/wizard_service_tests)
    add_test(NAME AuthServiceTests COMMAND ${CMAKE_BINARY_DIR}/tests/auth_service_tests)
    add_test(NAME InvitationServiceTests COMMAND ${CMAKE_BINARY_DIR}/tests/invitation_service_tests)
    add_test(NAME CapabilitiesTests COMMAND ${CMAKE_BINARY_DIR}/tests/capabilities_tests)
    add_test(NAME RoleServiceTests COMMAND ${CMAKE_BINARY_DIR}/tests/role_service_tests)
    add_test(NAME EndpointServiceTests COMMAND ${CMAKE_BINARY_DIR}/tests/endpoint_service_tests)
    add_test(NAME DynamicVariablesTests COMMAND ${CMAKE_BINARY_DIR}/tests/dynamic_variables_tests)
    add_test(NAME RequestExecutorTests COMMAND ${CMAKE_BINARY_DIR}/tests/request_executor_tests)
    add_test(NAME QuickJsSandboxTests COMMAND ${CMAKE_BINARY_DIR}/tests/quickjs_sandbox_tests)
endif()
