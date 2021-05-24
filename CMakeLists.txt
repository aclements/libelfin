cmake_minimum_required(VERSION 3.15)

project(libelfin)

set(CMAKE_CXX_STANDARD 11)

# build libelf and libdwarf
add_subdirectory(elf)
add_subdirectory(dwarf)

# build examples
add_subdirectory(examples)