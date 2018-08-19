cmake_minimum_required(VERSION 3.6)

project(libelfinexamples)

add_executable(dump-lines dump-lines.cc)
target_link_libraries(dump-lines dwarf)

add_executable(dump-sections dump-sections.cc)
target_link_libraries(dump-sections dwarf)

add_executable(dump-segments dump-segments.cc)
target_link_libraries(dump-segments dwarf)

add_executable(dump-syms dump-syms.cc)
target_link_libraries(dump-syms dwarf)

add_executable(dump-tree dump-tree.cc)
target_link_libraries(dump-tree dwarf)

add_executable(find-pc find-pc.cc)
target_link_libraries(find-pc dwarf)
