set(TARGET_NAME gtest_${PROJECT_NAME})

set(SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/bencode_parser_test.cpp)

find_package(GTest 1.11 REQUIRED)
find_package(Fmt 8.1 REQUIRED)

add_executable(${TARGET_NAME} ${SOURCES})

target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -std=c++20)

target_link_libraries(${TARGET_NAME} PRIVATE
    ${PROJECT_NAME}
    fmt
    GTest::gtest_main
)

include(GoogleTest)
gtest_add_tests(TARGET ${TARGET_NAME})
