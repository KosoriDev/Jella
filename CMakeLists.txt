cmake_minimum_required(VERSION 3.31)
project(jella CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(server main.cpp
        server.cpp
        server.h
        webpage_handler.cpp
        webpage_handler.h
        yaml/yaml.cpp
        yaml/yaml.hpp)

target_include_directories(server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (MINGW)
    target_link_options(server PRIVATE -static)
endif()

if (WIN32)
    target_link_libraries(server PRIVATE ws2_32)
endif ()

set_target_properties(server PROPERTIES OUTPUT_NAME "jella")