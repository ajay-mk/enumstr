cmake_minimum_required(VERSION 3.20)
project(enumstr CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Header-only library, consumed via target_link_libraries(... enumstr::enumstr).
add_library(enumstr INTERFACE)
add_library(enumstr::enumstr ALIAS enumstr)
target_include_directories(enumstr INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(enumstr INTERFACE cxx_std_20)

enable_testing()

add_executable(test_enum_names tests/test_enum_names.cpp)
target_link_libraries(test_enum_names PRIVATE enumstr::enumstr)
if(MSVC)
    target_compile_options(test_enum_names PRIVATE /W4 /WX)
else()
    target_compile_options(test_enum_names PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

add_test(NAME enum_names COMMAND test_enum_names)

install(TARGETS enumstr EXPORT enumstrTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT enumstrTargets
    FILE enumstrConfig.cmake
    NAMESPACE enumstr::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/enumstr)
