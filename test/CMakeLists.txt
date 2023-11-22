set(TARGET_NAME gtest_${PROJECT_NAME})

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/bencode_parser_test.cpp)

find_package(GTest REQUIRED)

add_executable(${TARGET_NAME} ${SOURCES})

target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -std=c++20)

target_link_libraries(${TARGET_NAME} PRIVATE
    ${PROJECT_NAME}
    GTest::gtest_main
)
