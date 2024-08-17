cmake_minimum_required(VERSION 3.15...3.30)

project(andromeda VERSION 1.0
                  DESCRIPTION "Andromeda Website"
                  LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS false)

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(andromeda src/main.cpp src/add.cpp)