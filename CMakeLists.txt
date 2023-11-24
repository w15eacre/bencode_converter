cmake_minimum_required(VERSION 3.16)

project(bencode_converter LANGUAGES CXX)

set(INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")

set(HEADERS "${INCLUDE_DIR}/bencode_parser.h")

add_library(${PROJECT_NAME} INTERFACE ${HEADERS})
target_include_directories(${PROJECT_NAME} INTERFACE
    ${INCLUDE_DIR}
)

enable_testing()

add_subdirectory(test)
