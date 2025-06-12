cmake_minimum_required(VERSION 3.25.1)
project(jella LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OPENSSL_USE_STATIC_LIBS TRUE)

find_package(OpenSSL 3.0 REQUIRED COMPONENTS Crypto SSL)

add_executable(jella
        main.cpp
        server.cpp server.h
        webpage_handler.cpp webpage_handler.h
        yaml/Yaml.cpp yaml/Yaml.hpp
        mime_types_data.h.in
)

file(STRINGS "mime_types.csv" mime_lines)
list(LENGTH mime_lines MIME_TYPES_CSV_SIZE)

set(MIME_TYPES_CSV_SIZE ${MIME_TYPES_CSV_SIZE})

configure_file(
        "mime_types_data.h.in"
        "${CMAKE_BINARY_DIR}/mime_types_data.h"
)

target_link_libraries(jella PRIVATE
        OpenSSL::SSL
)

if (WIN32)
    target_link_libraries(jella PRIVATE ws2_32)
endif()

if (MINGW)
    target_link_options(jella PRIVATE -static -static-libgcc -static-libstdc++)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(jella PRIVATE ws2_32)
endif()

set_target_properties(jella PROPERTIES
        OUTPUT_NAME "jella"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(
        jella PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
)