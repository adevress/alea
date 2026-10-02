
## Code style instructions
- Use '//' comment style and '///' for documentation
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

## Restrictions

Respect these rules:
 - Never write or rewrite git history without the explicit and direct demand of the user. That also concerns commit, push, and rebase.
 - If you do need a tool or a library, try to use Nix. Do not install things without Nix without the explicit demand of the user.

For the previous restrictions. Always ask a question if you doubt.
