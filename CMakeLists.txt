project(alea)
cmake_minimum_required(VERSION 3.10)


include(GNUInstallDirs)
include(FindPkgConfig)
include(CTest)

find_package(Boost 1.41.0 QUIET REQUIRED system unit_test_framework)

## Enforce CXX standard
set (CMAKE_CXX_STANDARD 17)


# headers
file(GLOB ALEA_HEADERS_1 "${CMAKE_CURRENT_SOURCE_DIR}/include/alea/*.hpp")
file(GLOB ALEA_HEADERS_2 "${CMAKE_CURRENT_SOURCE_DIR}/include/alea/*/*.hpp")
list(APPEND ALEA_HEADERS ${ALEA_HEADERS_1} ${ALEA_HEADERS_2})
set(ALEA_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include")


pkg_check_modules(CATCH2 IMPORTED_TARGET catch2)



# unit tests on alea
list(APPEND test_random_src "tests/test_random.cpp")
add_executable(test_random ${test_random_src} ${ALEA_HEADERS})
target_include_directories(test_random PRIVATE ${ALEA_INCLUDE_DIRS} )
target_link_libraries(test_random PRIVATE Boost::unit_test_framework)
target_compile_definitions(test_random PRIVATE "-DBOOST_TEST_DYN_LINK=TRUE")

add_test(NAME test_random_unit COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_random)

# perf tests on alea
list(APPEND test_perf_random_src "tests/random_perf.cpp")
add_executable(perf_random ${test_perf_random_src} ${ALEA_HEADERS})
target_include_directories(perf_random PRIVATE ${ALEA_INCLUDE_DIRS})
target_link_libraries(perf_random PRIVATE Boost::unit_test_framework)
target_compile_definitions(perf_random PRIVATE "-DBOOST_TEST_DYN_LINK=TRUE")

add_test(NAME perf_random_unit COMMAND ${CMAKE_CURRENT_BINARY_DIR}/perf_random)
