cmake_minimum_required(VERSION 3.21)
project(ALEA VERSION 0.1.0 LANGUAGES CXX)

option(ALEA_ENABLE_TESTS "Build the alea unit tests" ${PROJECT_IS_TOP_LEVEL})
option(ALEA_ENABLE_BENCH "Build the alea benchmarks" ${PROJECT_IS_TOP_LEVEL})
option(ALEA_ENABLE_INSTALL "Enable the installation of the alea headers" ${PROJECT_IS_TOP_LEVEL})

# alea is a header only library: everything lives in include/alea
add_library(ALEA INTERFACE)
add_library(alea::alea ALIAS ALEA)

target_include_directories(ALEA INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                         $<INSTALL_INTERFACE:include>)
target_compile_features(ALEA INTERFACE cxx_std_20)

if(ALEA_ENABLE_TESTS)
    enable_testing()

    # doctest as a submodule, only used to build the tests: it must not be
    # installed together with alea
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/doctest)

    add_executable(ALEA_Test tests/threefry_test.cpp)
    target_link_libraries(ALEA_Test PRIVATE ALEA doctest::doctest)

    add_test(NAME ALEA_Test COMMAND ALEA_Test)
endif()

if(ALEA_ENABLE_BENCH)
    # nanobench as a submodule
    add_subdirectory(third_party/nanobench)

    add_executable(ALEA_Bench bench/alea_nano.cpp)
    target_link_libraries(ALEA_Bench PRIVATE ALEA nanobench::nanobench)
endif()

if(ALEA_ENABLE_INSTALL)
    include(GNUInstallDirs)

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/alea DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # alea is header only: it is discovered through pkg-config, like any other
    # C++ header only library
    configure_file("${PROJECT_SOURCE_DIR}/misc/alea.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/alea.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/alea.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
