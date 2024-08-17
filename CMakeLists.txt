cmake_minimum_required(VERSION 3.15...3.30)

project(andromeda VERSION 1.0
                  DESCRIPTION "Andromeda Website"
                  LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS false)

add_library(mongoose STATIC lib/mongoose/mongoose.c)

add_executable(andromeda src/main.cpp src/server.cpp src/handler.cpp)
target_compile_options(andromeda PRIVATE -Wall -Wextra -Wpedantic)
target_include_directories(andromeda PRIVATE lib)

target_link_libraries(andromeda mongoose)