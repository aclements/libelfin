add_library(libdwarf abbrev.cc attrs.cc cursor.cc die.cc 
	die_str_map.cc dwarf.cc elf.cc expr.cc line.cc rangelist.cc to_string.cc value.cc)

# add our directory to includes
target_include_directories(libdwarf
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# we need libelf
target_link_libraries(libdwarf
	PUBLIC libelfin::libelf)

# add an alias
add_library(libelfin::libdwarf ALIAS libdwarf)