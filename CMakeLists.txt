cmake_minimum_required(VERSION 3.25.1)
project(jella CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable(server main.cpp
        server.cpp
        server.h
        webpage_handler.cpp
        webpage_handler.h
        yaml/Yaml.cpp
        yaml/Yaml.hpp)

target_include_directories(server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

target_link_libraries(server ${OPENSSL_LIBRARIES})

if (MINGW)
    target_link_options(server PRIVATE -static)
endif()

if (WIN32)
    target_link_libraries(server PRIVATE ws2_32)
endif ()

set_target_properties(server PROPERTIES OUTPUT_NAME "jella")