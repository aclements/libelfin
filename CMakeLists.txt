cmake_minimum_required(VERSION 3.6)

project (elfin)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(elf elfdir)
add_subdirectory(dwarf dwarfdir)
add_subdirectory(examples examplesdir)
