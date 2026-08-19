cmake_minimum_required(VERSION 3.20)
project(enumstr VERSION 0.1.0 LANGUAGES CXX)

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

# Tests are for developing enumstr, not for consuming it: skip them when this
# is a FetchContent/add_subdirectory child of someone else's build.
if(PROJECT_IS_TOP_LEVEL)
    enable_testing()

    add_executable(test_enum_names tests/test_enum_names.cpp)
    target_link_libraries(test_enum_names PRIVATE enumstr::enumstr)
    if(MSVC)
        target_compile_options(test_enum_names PRIVATE /W4 /WX)
    else()
        target_compile_options(test_enum_names PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_test(NAME enum_names COMMAND test_enum_names)

    # Built so it cannot rot, but not registered with ctest: timings in CI are
    # noise. Run it by hand, at -O2, on an idle machine.
    add_executable(bench tests/bench.cpp)
    target_link_libraries(bench PRIVATE enumstr::enumstr)
endif()

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/enumstrConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT)

install(TARGETS enumstr EXPORT enumstrTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT enumstrTargets
    FILE enumstrConfig.cmake
    NAMESPACE enumstr::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/enumstr)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/enumstrConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/enumstr)
