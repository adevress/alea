cmake_minimum_required(VERSION 3.21)
project(ALEA LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


file(GLOB alea_incs "${CMAKE_CURRENT_SOURCE_DIR}/include/alea/*.hpp")

# Add library
add_library(ALEA)
set_target_properties(ALEA PROPERTIES LINKER_LANGUAGE CXX)

target_sources(ALEA PUBLIC ${alea_incs} PRIVATE ${alea_incs})
target_include_directories(ALEA PUBLIC include/)



# doctest as submodule
add_subdirectory(third_party/doctest)

# Add test executable
add_executable(ALEA_Test
    tests/threefry_test.cpp
    # Add more test files as needed
)

# Link library to test executable
target_link_libraries(ALEA_Test PRIVATE ALEA)

# Link doctest to the test executable
target_link_libraries(ALEA_Test PRIVATE ALEA doctest::doctest)

# Define the test target
add_test(NAME ALEA_Test COMMAND ALEA_Test)


# nanobench as submodule
add_subdirectory(third_party/nanobench)

# add basic benchmark
add_executable(ALEA_Bench
    "bench/alea_nano.cpp"
)
target_link_libraries(ALEA_Bench PRIVATE ALEA nanobench::nanobench)
