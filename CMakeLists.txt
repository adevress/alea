# CMake 3.28 is the version shipped with Ubuntu 24.04 LTS (cmake 3.28.3),
# the lowest supported baseline.
cmake_minimum_required(VERSION 3.28)

# alea uses no C++20 modules: disable the clang module dependency scan that
# CMake >= 3.28 otherwise runs on every translation unit with the Ninja
# generator. It is not only unnecessary, it is also broken with toolchain
# wrappers that pass include dirs through the environment (e.g. Nix).
set(CMAKE_CXX_SCAN_FOR_MODULES OFF CACHE BOOL "Disable C++20 module dependency scanning" OFF)

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
option(ALEA_ENABLE_INSTALL "Install the alea headers" ${PROJECT_IS_TOP_LEVEL})
option(ALEA_ENABLE_BENCH "Build the alea benchmarks (nanobench submodule)" ${PROJECT_IS_TOP_LEVEL})
option(ALEA_ENABLE_VALIDATION "Validate alea against the reference random123 implementation, fetched with FetchContent" OFF)
option(ALEA_ENABLE_CUDA "Build the alea CUDA tests and benchmarks" OFF)

# alea is a header only library: everything lives in include/alea
add_library(ALEA INTERFACE)
add_library(alea::alea ALIAS ALEA)

target_include_directories(ALEA INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                         $<INSTALL_INTERFACE:include>)
target_compile_features(ALEA INTERFACE cxx_std_20)

# alea uses std::array and std::numeric_limits, which are constexpr but host
# only: nvcc needs relaxed constexpr to call them from device code. The flag
# is attached to the interface target so that it reaches every CUDA consumer
# of alea, whether or not the alea GPU targets themselves are enabled.
target_compile_options(ALEA INTERFACE $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>)

if(ALEA_ENABLE_CUDA)
    # when the architectures are not specified, compile for the GPU of the
    # build machine so that the tests can run; override with
    # -DCMAKE_CUDA_ARCHITECTURES=... to target other devices. This must be
    # set before enable_language(CUDA), which otherwise pins the nvcc default.
    if(NOT CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES native CACHE STRING "CUDA architectures to build for" FORCE)
        set_property(CACHE CMAKE_CUDA_ARCHITECTURES PROPERTY STRINGS native)
    endif()

    # the same headers are compiled by nvcc: enable the CUDA language and
    # propagate the flags the single source implementation needs to every
    # consumer of the ALEA interface target
    enable_language(CUDA)

    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

if(ALEA_ENABLE_TESTS)
    enable_testing()

    # doctest as a submodule, only used to build the tests: it must not be
    # installed together with alea. Its own test suite is already skipped
    # because doctest is added as a subproject (MAIN_PROJECT is false).
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
    add_subdirectory(third_party/doctest)

    add_executable(ALEA_Test tests/threefry_test.cpp)
    target_link_libraries(ALEA_Test PRIVATE ALEA doctest::doctest)
    add_test(NAME ALEA_Test COMMAND ALEA_Test)

    add_executable(ALEA_Philox_Test tests/philox_test.cpp)
    target_link_libraries(ALEA_Philox_Test PRIVATE ALEA doctest::doctest)
    add_test(NAME ALEA_Philox_Test COMMAND ALEA_Philox_Test)

    if(ALEA_ENABLE_CUDA)
        add_executable(ALEA_GPU_Test tests/gpu/gpu_test.cu)
        target_link_libraries(ALEA_GPU_Test PRIVATE ALEA doctest::doctest)
        add_test(NAME ALEA_GPU_Test COMMAND ALEA_GPU_Test)
    endif()

    if(ALEA_ENABLE_VALIDATION)
        include(FetchContent)

        # the reference random123 implementation is fetched at configure time,
        # it is only used to build the validation tests and must not be
        # installed together with alea. random123 is header only and ships no
        # CMakeLists.txt: FetchContent_MakeAvailable only populates it and
        # exposes random123_SOURCE_DIR, which is all we need.
        FetchContent_Declare(
          random123
          GIT_REPOSITORY https://github.com/DEShawResearch/random123
          GIT_TAG v1.14.0
          GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(random123)

        # small helper to add one validation test against random123
        function(alea_add_validation_test name source)
            add_executable(${name} ${source})
            target_link_libraries(${name} PRIVATE ALEA doctest::doctest)
            target_include_directories(${name} PRIVATE ${random123_SOURCE_DIR}/include)
            add_test(NAME ${name} COMMAND ${name})
        endfunction()

        alea_add_validation_test(ALEA_Threefry_Validation tests/validation/threefry_validation.cpp)
        alea_add_validation_test(ALEA_Philox_Validation tests/validation/philox_validation.cpp)

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
            alea_add_validation_test(ALEA_Philox_Fallback_Validation tests/validation/philox_fallback_validation.cpp)
            target_compile_definitions(ALEA_Philox_Fallback_Validation PRIVATE ALEA_PHILOX_DISABLE_INT128)
        endif()
    endif()
endif()

if(ALEA_ENABLE_BENCH)
    # nanobench as a submodule, only used to build the benchmarks: it must not
    # be installed together with alea and, as a subproject, it does not build
    # its own test suite.
    add_subdirectory(third_party/nanobench)

    add_executable(ALEA_Bench bench/alea_nano.cpp)
    target_link_libraries(ALEA_Bench PRIVATE ALEA nanobench::nanobench)

    if(ALEA_ENABLE_CUDA)
        add_executable(ALEA_GPU_Bench bench/alea_nano_gpu.cu)
        target_link_libraries(ALEA_GPU_Bench PRIVATE ALEA nanobench::nanobench)
    endif()
endif()

if(ALEA_ENABLE_INSTALL)
    include(GNUInstallDirs)

    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/alea DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    # alea is header only: it is discovered through pkg-config, like any other
    # C++ header only library
    configure_file("${PROJECT_SOURCE_DIR}/misc/alea.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/alea.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/alea.pc" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
