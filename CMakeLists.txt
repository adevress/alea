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
option(ALEA_ENABLE_VALIDATION_BLAKE2 "Validate alea against the reference BLAKE2 implementation, fetched with FetchContent" OFF)
option(ALEA_ENABLE_VALIDATION_SHA256 "Validate alea against a reference SHA-256 implementation, fetched with FetchContent" OFF)
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

# ARS is built on the AES round function: on x86 the AES-NI instructions, on
# AArch64 the Cryptography Extensions. The headers select the optimized
# implementation from the predefined compiler macros (__AES__ and
# __ARM_FEATURE_AES), but the compiler must first be told that the target
# supports the instructions. The tests and the benchmarks turn them on when
# the build machine is actually able to execute them, so that both the
# optimized path and the portable fallback are exercised.
if(ALEA_ENABLE_TESTS OR ALEA_ENABLE_BENCH)
    include(CheckCXXSourceRuns)

    set(ALEA_AES_ARCH_OPTION "")

    set(CMAKE_REQUIRED_FLAGS "-maes")
    check_cxx_source_runs(
      "
#include <wmmintrin.h>
int main() {
  __m128i a = _mm_setzero_si128();
  a = _mm_aesenc_si128(a, a);
  volatile int x = _mm_cvtsi128_si32(a);
  (void)x;
  return 0;
}
"
      ALEA_HAVE_AESNI)

    if(ALEA_HAVE_AESNI)
        set(ALEA_AES_ARCH_OPTION "-maes")
    else()
        set(CMAKE_REQUIRED_FLAGS "-march=armv8-a+crypto")
        check_cxx_source_runs(
          "
#include <arm_neon.h>
int main() {
  uint8x16_t a = vdupq_n_u8(0);
  a = vaeseq_u8(a, a);
  volatile unsigned char x = vgetq_lane_u8(a, 0);
  (void)x;
  return 0;
}
"
          ALEA_HAVE_ARM_CRYPTO)

        if(ALEA_HAVE_ARM_CRYPTO)
            set(ALEA_AES_ARCH_OPTION "-march=armv8-a+crypto")
        endif()
    endif()

    unset(CMAKE_REQUIRED_FLAGS)

    # interface target that enables the AES instructions for the targets that
    # exercise the optimized ARS path; it carries no flag on a platform
    # without a usable AES instruction set
    add_library(ALEA_AES_ARCH INTERFACE)
    if(ALEA_AES_ARCH_OPTION)
        target_compile_options(ALEA_AES_ARCH INTERFACE ${ALEA_AES_ARCH_OPTION})
    endif()
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

    add_executable(ALEA_Ars_Test tests/ars_test.cpp)
    target_link_libraries(ALEA_Ars_Test PRIVATE ALEA doctest::doctest ALEA_AES_ARCH)
    add_test(NAME ALEA_Ars_Test COMMAND ALEA_Ars_Test)

    add_executable(ALEA_Blake2_Test tests/blake2_test.cpp)
    target_link_libraries(ALEA_Blake2_Test PRIVATE ALEA doctest::doctest)
    add_test(NAME ALEA_Blake2_Test COMMAND ALEA_Blake2_Test)

    add_executable(ALEA_Sha256_Test tests/sha256_test.cpp)
    target_link_libraries(ALEA_Sha256_Test PRIVATE ALEA doctest::doctest)
    add_test(NAME ALEA_Sha256_Test COMMAND ALEA_Sha256_Test)

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

        # the ARS validation relies on the official Random123 known answers on
        # every platform, and on a direct comparison with r123::ARS4x32_R
        # when the reference implementation can use AES-NI. The AES
        # instructions are enabled for these targets so that both the
        # optimized path and the fallback are validated against the reference
        alea_add_validation_test(ALEA_Ars_Validation tests/validation/ars_validation.cpp)
        target_link_libraries(ALEA_Ars_Validation PRIVATE ALEA_AES_ARCH)

        # the portable software fallback is validated on its own through an
        # additional translation unit compiled with ALEA_ARS_DISABLE_HW
        alea_add_validation_test(ALEA_Ars_Fallback_Validation tests/validation/ars_fallback_validation.cpp)
        target_link_libraries(ALEA_Ars_Fallback_Validation PRIVATE ALEA_AES_ARCH)
        target_compile_definitions(ALEA_Ars_Fallback_Validation PRIVATE ALEA_ARS_DISABLE_HW)

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

if(ALEA_ENABLE_TESTS AND ALEA_ENABLE_VALIDATION_BLAKE2)
    include(FetchContent)

    # the reference BLAKE2 implementation is fetched at configure time, it is
    # only used to build the validation test and must not be installed
    # together with alea. The commit is pinned: the BLAKE2 repository has no
    # tags newer than 2019 and its HEAD moved after the last release.
    FetchContent_Declare(
      blake2
      GIT_REPOSITORY https://github.com/BLAKE2/BLAKE2
      GIT_TAG ed1974ea83433eba7b2d95c5dcd9ac33cb847913
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(blake2)

    # the reference sources are C files compiled as C for the validation
    # executable. Both carry a main() selftest, so the two entry points are
    # renamed on the command line of these two files only, and the C sources
    # are put in their own static library so that the rename never touches
    # the C++ test
    enable_language(C)

    add_library(alea_blake2_reference STATIC ${blake2_SOURCE_DIR}/ref/blake2b-ref.c
                                              ${blake2_SOURCE_DIR}/ref/blake2s-ref.c)
    target_include_directories(alea_blake2_reference PUBLIC ${blake2_SOURCE_DIR}/ref)
    target_compile_options(alea_blake2_reference PRIVATE -w)
    set_source_files_properties(${blake2_SOURCE_DIR}/ref/blake2b-ref.c PROPERTIES COMPILE_DEFINITIONS
                                                                                       "main=blake2b_reference_main")
    set_source_files_properties(${blake2_SOURCE_DIR}/ref/blake2s-ref.c PROPERTIES COMPILE_DEFINITIONS
                                                                                       "main=blake2s_reference_main")

    add_executable(ALEA_Blake2_Validation tests/validation/blake2_validation.cpp)
    target_link_libraries(ALEA_Blake2_Validation PRIVATE ALEA doctest::doctest alea_blake2_reference)
    add_test(NAME ALEA_Blake2_Validation COMMAND ALEA_Blake2_Validation)
endif()

if(ALEA_ENABLE_TESTS AND ALEA_ENABLE_VALIDATION_SHA256)
    include(FetchContent)

    # a reference SHA-256 implementation is fetched at configure time, it is
    # only used to build the validation test and must not be installed
    # together with alea. The classic pedagogical implementation of Brad
    # Conte, in wide use as a reference for FIPS 180-4, is pinned to its
    # latest commit.
    FetchContent_Declare(
      crypto-algorithms
      GIT_REPOSITORY https://github.com/B-Con/crypto-algorithms
      GIT_TAG cfbde48414baacf51fc7c74f275190881f037d32
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(crypto-algorithms)

    # the reference source is a C file compiled as C for the validation
    # executable
    enable_language(C)

    add_executable(ALEA_Sha256_Validation tests/validation/sha256_validation.cpp
                                          ${crypto-algorithms_SOURCE_DIR}/sha256.c)
    target_include_directories(ALEA_Sha256_Validation PRIVATE ${crypto-algorithms_SOURCE_DIR})
    target_compile_options(ALEA_Sha256_Validation PRIVATE -w)
    target_link_libraries(ALEA_Sha256_Validation PRIVATE ALEA doctest::doctest)
    add_test(NAME ALEA_Sha256_Validation COMMAND ALEA_Sha256_Validation)
endif()

if(ALEA_ENABLE_BENCH)
    # nanobench as a submodule, only used to build the benchmarks: it must not
    # be installed together with alea and, as a subproject, it does not build
    # its own test suite.
    add_subdirectory(third_party/nanobench)

    add_executable(ALEA_Bench bench/alea_nano.cpp)
    target_link_libraries(ALEA_Bench PRIVATE ALEA nanobench::nanobench ALEA_AES_ARCH)

    add_executable(ALEA_Hash_Bench bench/alea_hash.cpp)
    target_link_libraries(ALEA_Hash_Bench PRIVATE ALEA nanobench::nanobench)

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
