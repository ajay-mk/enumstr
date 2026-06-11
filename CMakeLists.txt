cmake_minimum_required(VERSION 3.20)
project(enumstr CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only library, consumed via target_link_libraries(... enum_names).
add_library(enum_names INTERFACE)
target_include_directories(enum_names INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

enable_testing()

add_executable(test_enum_names tests/test_enum_names.cpp)
target_link_libraries(test_enum_names PRIVATE enum_names)
target_compile_options(test_enum_names PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_test(NAME enum_names COMMAND test_enum_names)
