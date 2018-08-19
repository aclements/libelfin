cmake_minimum_required(VERSION 3.6)

project(dwarf)

set(dwarfH ${CMAKE_CURRENT_LIST_DIR}/dwarf++.hh)
set(dataH ${CMAKE_CURRENT_LIST_DIR}/data.hh)
set(to_string_src ${CMAKE_CURRENT_BINARY_DIR}/to_string.cc)
set(configured_to_string_script ${CMAKE_CURRENT_BINARY_DIR}/create_to_string_code.sh)

configure_file(create_to_string_code.sh ${configured_to_string_script})

add_custom_command(
  OUTPUT ${to_string_src}
  COMMAND bash ${configured_to_string_script} ${dwarfH} ${dataH} ${to_string_src})
add_custom_target(to_string_target DEPENDS ${to_string_src})

add_library(dwarf dwarf.cc cursor.cc die.cc value.cc abbrev.cc
  expr.cc rangelist.cc line.cc attrs.cc
  die_str_map.cc elf.cc ${to_string_src})
target_include_directories(dwarf PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(dwarf elf)
