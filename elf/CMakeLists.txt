cmake_minimum_required(VERSION 3.6)

project(elf)

set(toStringInputHeader ${CMAKE_CURRENT_LIST_DIR}/data.hh)
set(to_string_src ${CMAKE_CURRENT_BINARY_DIR}/to_string.cc)
set(configured_to_string_script ${CMAKE_CURRENT_BINARY_DIR}/create_to_string_code.sh)

configure_file(create_to_string_code.sh ${configured_to_string_script})

add_custom_command(
  OUTPUT ${to_string_src}
  COMMAND bash ${configured_to_string_script} ${toStringInputHeader} ${to_string_src})
add_custom_target(elf_to_string_target DEPENDS ${to_string_src})
add_library(elf elf.cc mmap_loader.cc ${to_string_src})
add_dependencies(elf elf_to_string_target)
target_include_directories(elf PUBLIC ${CMAKE_CURRENT_LIST_DIR})
