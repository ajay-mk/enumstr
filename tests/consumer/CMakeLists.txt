cmake_minimum_required(VERSION 3.21)
project(enumstr_consumer LANGUAGES CXX)

add_subdirectory(${enumstr_SOURCE_DIR} enumstr)

if(TARGET test_enum_names OR TARGET bench)
    message(FATAL_ERROR "enumstr added project-only targets to a consumer build")
endif()

add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE enumstr::enumstr)

foreach(case IN ITEMS invalid_range oversized_range)
    add_executable(${case} EXCLUDE_FROM_ALL ${case}.cpp)
    target_link_libraries(${case} PRIVATE enumstr::enumstr)
endforeach()
