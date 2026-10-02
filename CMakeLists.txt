cmake_minimum_required(VERSION 3.21)
project(ALEA VERSION 0.1.0 LANGUAGES CXX)

# default to an optimized build with debug symbols when alea is built as the
# top level project and no build type was requested: it is the user's
# responsibility to pick another build type, for example
# -DCMAKE_BUILD_TYPE=Debug
if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

option(ALEA_ENABLE_TESTS "Build the alea unit tests" ${PROJECT_IS_TOP_LEVEL})
option(ALEA_ENABLE_INSTALL "Enable the installation of the alea headers" ${PROJECT_IS_TOP_LEVEL})
option(ENABLE_BENCH "Build the alea benchmarks, nanobench is fetched with FetchContent" ${PROJECT_IS_TOP_LEVEL})
option(ENABLE_VALIDATION "Validate alea against the reference random123 implementation, fetched with FetchContent" OFF)

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

    add_executable(ALEA_Philox_Test tests/philox_test.cpp)
    target_link_libraries(ALEA_Philox_Test PRIVATE ALEA doctest::doctest)

    add_test(NAME ALEA_Philox_Test COMMAND ALEA_Philox_Test)

    if(ENABLE_VALIDATION)
        include(FetchContent)

        # the reference random123 implementation is fetched at configure
        # time, it is only used to build the validation tests and must not
        # be installed together with alea
        FetchContent_Declare(
          random123
          GIT_REPOSITORY https://github.com/DEShawResearch/random123
          GIT_TAG v1.14.0
          GIT_SHALLOW TRUE
        )

        # random123 is a header only library and does not ship any
        # CMakeLists.txt: we only fetch its sources and add its include
        # directory to the validation tests
        FetchContent_GetProperties(random123)
        if(NOT random123_POPULATED)
            FetchContent_Populate(random123)
        endif()

        add_executable(ALEA_Validation tests/validation/threefry_validation.cpp)
        target_link_libraries(ALEA_Validation PRIVATE ALEA doctest::doctest)
        target_include_directories(ALEA_Validation PRIVATE ${random123_SOURCE_DIR}/include)

        add_test(NAME ALEA_Validation COMMAND ALEA_Validation)

        add_executable(ALEA_Philox_Validation tests/validation/philox_validation.cpp)
        target_link_libraries(ALEA_Philox_Validation PRIVATE ALEA doctest::doctest)
        target_include_directories(ALEA_Philox_Validation PRIVATE ${random123_SOURCE_DIR}/include)

        add_test(NAME ALEA_Philox_Validation COMMAND ALEA_Philox_Validation)

        # the fast 64 bit philox multiply relies on a double width integer
        # type. When the platform provides one, the portable fallback is
        # validated as well through an additional translation unit compiled
        # with ALEA_PHILOX_DISABLE_INT128, against the same reference
        include(CheckCXXSourceCompiles)
        check_cxx_source_compiles(
          "
#if !defined(__SIZEOF_INT128__)
#error the compiler does not provide a 128 bit integer type
#endif
int main() { return 0; }
"
          ALEA_HAVE_INT128)

        if(ALEA_HAVE_INT128)
            add_executable(ALEA_Philox_Fallback_Validation tests/validation/philox_fallback_validation.cpp)
            target_link_libraries(ALEA_Philox_Fallback_Validation PRIVATE ALEA doctest::doctest)
            target_include_directories(ALEA_Philox_Fallback_Validation PRIVATE ${random123_SOURCE_DIR}/include)
            target_compile_definitions(ALEA_Philox_Fallback_Validation PRIVATE ALEA_PHILOX_DISABLE_INT128)

            add_test(NAME ALEA_Philox_Fallback_Validation COMMAND ALEA_Philox_Fallback_Validation)
        endif()
    endif()
endif()

if(ENABLE_BENCH)
    include(FetchContent)

    # nanobench is fetched at configure time, it is only used to build the
    # benchmarks and must not be installed together with alea
    FetchContent_Declare(
      nanobench
      GIT_REPOSITORY https://github.com/martinus/nanobench.git
      GIT_TAG v4.6.0
      GIT_SHALLOW TRUE
    )

    # nanobench only defines its targets when it is pulled in with
    # add_subdirectory, which is exactly what FetchContent_MakeAvailable does
    FetchContent_MakeAvailable(nanobench)

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
