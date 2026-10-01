
## Code style instructions
- Use '//' comment style and '///' for documentations
- Use `pragma once` instead of include guards
- Respect the existing namespace hierarchy
- never use `using namespace` in C++
- Types follow the STL and Random123 conventions: lowercase names (`threefry`, `counter_engine`, `threefry4x64`), not CamelCase
- functions, variables and template parameters are in snakecase
- do not use pre-increment and post-increment syntax outside of for loops
- For the rest, in C++ use a style similar to the CPP core guidelines

## Compilation instructions
 - Create build directory `mkdir -p build-agent`
 - Run `cd build-agent && cmake -G Ninja ../ && ninja -v`


## Test instructions
 - Run `ctest -V`

## Formatting instructions
- Run `task format` (it uses the `.clang-format` file at the root of the repository)
- Run `task format-check` to verify the formatting without modifying the files


