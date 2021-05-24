add_executable(dump-lines dump-lines.cc)

target_link_libraries(dump-lines
	PUBLIC libelfin::libdwarf)

add_executable(dump-sections dump-sections.cc)

target_link_libraries(dump-sections
	PUBLIC libelfin::libdwarf)

add_executable(dump-segments dump-segments.cc)

target_link_libraries(dump-segments
	PUBLIC libelfin::libdwarf)

add_executable(dump-syms dump-syms.cc)

target_link_libraries(dump-syms
	PUBLIC libelfin::libdwarf)

add_executable(dump_tree dump-tree.cc)

target_link_libraries(dump_tree
	PUBLIC libelfin::libdwarf)

add_executable(find_pc find-pc.cc)

target_link_libraries(find_pc
	PUBLIC libelfin::libdwarf)