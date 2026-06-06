cmake_minimum_required(VERSION 3.19...4.3)

project(andromeda VERSION 1.0
                  DESCRIPTION "Andromeda Website"
                  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS false)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS true)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(mongoose STATIC lib/mongoose/mongoose.c)
target_compile_definitions(mongoose PRIVATE
    MG_IO_SIZE=32768 MG_ENABLE_IPV6=1
    MG_ENABLE_DIRLIST=0
    MG_TLS=MG_TLS_MBED
    MG_ENABLE_CUSTOM_RANDOM=1
)
target_link_libraries(mongoose PRIVATE
    MbedTLS::mbedtls
    MbedTLS::mbedcrypto
    MbedTLS::mbedx509
)

add_library(sqlite STATIC lib/sqlite/sqlite3.c)
target_compile_options(sqlite PRIVATE -Wno-language-extension-token)

add_library(inja INTERFACE)
target_include_directories(inja INTERFACE
    lib/inja/single_include
)

add_compile_definitions(MBEDTLS_PSA_CRYPTO_CONFIG)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING  OFF CACHE BOOL "" FORCE)
add_subdirectory(lib/mbedtls)
add_subdirectory(lib/base64)
add_subdirectory(lib/plog)

set(JSON_BuildTests OFF CACHE INTERNAL "")
set(JSON_Install OFF CACHE INTERNAL "")
add_subdirectory(lib/json)

add_executable(andromeda
    src/main.cpp
    src/server.cpp
    src/handler.cpp
    src/db.cpp
    src/crypto.cpp
    src/util.cpp
    src/config.cpp
    src/auth.cpp
    src/ratelimit.cpp
    src/handlers/index.cpp
    src/handlers/game.cpp
    src/handlers/about.cpp
    src/handlers/login.cpp
    src/handlers/register.cpp
    src/handlers/discord.cpp
    src/handlers/og.cpp
    src/handlers/short.cpp
)
target_compile_options(andromeda PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-language-extension-token
    -Wno-c99-extensions
)
target_include_directories(andromeda PRIVATE lib)
if(WIN32)
    target_compile_definitions(andromeda PRIVATE NOMINMAX)
endif()

target_link_libraries(andromeda PRIVATE
    mongoose
    sqlite
    MbedTLS::mbedtls
    MbedTLS::mbedcrypto
    MbedTLS::mbedx509
    base64
    inja
    nlohmann_json::nlohmann_json
    plog::plog
)
