add_library(libelf elf.cc mmap_loader.cc to_string.cc)

# add our directory to includes
target_include_directories(libelf
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# add an alias library
add_library(libelfin::libelf ALIAS libelf)