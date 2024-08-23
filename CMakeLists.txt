cmake_minimum_required(VERSION 3.15...3.30)

project(andromeda VERSION 1.0
                  DESCRIPTION "Andromeda Website"
                  LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS false)

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

add_subdirectory(lib/mbedtls)
add_subdirectory(lib/base64)

add_executable(andromeda
    src/main.cpp
    src/server.cpp
    src/handler.cpp
    src/db.cpp
    src/crypto.cpp
    src/util.cpp
    src/config.cpp
    src/handlers/index.cpp
    src/handlers/game.cpp
    src/handlers/about.cpp
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
)
