cmake_minimum_required(VERSION 3.20)

project(enumix
    VERSION 0.1.0
    DESCRIPTION "Simple constexpr enum/string utilities for C++20"
    LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(ENUMIX_BUILD_TESTS "Build enumix tests" ${PROJECT_IS_TOP_LEVEL})
option(ENUMIX_BUILD_EXAMPLES "Build enumix examples" ${PROJECT_IS_TOP_LEVEL})
option(ENUMIX_FETCH_CATCH2 "Fetch Catch2 automatically when tests are enabled" ${PROJECT_IS_TOP_LEVEL})

add_library(enumix INTERFACE)
add_library(enumix::enumix ALIAS enumix)

target_compile_features(enumix INTERFACE cxx_std_20)
target_include_directories(enumix
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

function(enumix_enable_msvc_exceptions target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /EHsc)
    endif()
endfunction()

install(TARGETS enumix EXPORT enumixTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT enumixTargets
    FILE enumixTargets.cmake
    NAMESPACE enumix::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/enumix)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/enumixConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/enumixConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/enumixConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/enumix")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/enumixConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/enumixConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/enumix")

if(ENUMIX_BUILD_EXAMPLES)
    add_executable(enumix_basic_example examples/basic.cpp)
    target_link_libraries(enumix_basic_example PRIVATE enumix::enumix)
    enumix_enable_msvc_exceptions(enumix_basic_example)
endif()

if(ENUMIX_BUILD_TESTS)
    include(CTest)

    if(BUILD_TESTING)
        find_package(Catch2 3 QUIET)

        if(NOT Catch2_FOUND)
            if(NOT ENUMIX_FETCH_CATCH2)
                message(FATAL_ERROR
                    "Catch2 3 was not found. Install Catch2 or configure with -DENUMIX_FETCH_CATCH2=ON.")
            endif()

            include(FetchContent)
            FetchContent_Declare(
                Catch2
                URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.7.1.tar.gz)
            FetchContent_MakeAvailable(Catch2)
        endif()

        add_executable(enumix_tests tests/enumix_tests.cpp)
        target_link_libraries(enumix_tests PRIVATE enumix::enumix Catch2::Catch2WithMain)
        enumix_enable_msvc_exceptions(enumix_tests)

        include(Catch)
        catch_discover_tests(enumix_tests)
    endif()
endif()
