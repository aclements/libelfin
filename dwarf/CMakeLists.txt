project(dwarf++)

add_custom_command(
        OUTPUT to_string.cc
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gen_to_string.sh
        ${CMAKE_CURRENT_BINARY_DIR}/to_string.cc
        DEPENDS ../elf/enum-print.py gen_to_string.sh
)

set(DWARF_PP_SRC
        abbrev.cc
        attrs.cc
        cursor.cc
        die.cc
        die_str_map.cc
        dwarf.cc
        elf.cc
        expr.cc
        line.cc
        rangelist.cc
        value.cc
        ${CMAKE_CURRENT_BINARY_DIR}/to_string.cc
        )

set(DWARF_PP_HEADERS
        data.hh
        dwarf++.hh
        internal.hh
        small_vector.hh
        )

add_library(
        ${PROJECT_NAME}_static STATIC
        ${DWARF_PP_SRC}
        ${DWARF_PP_HEADERS}
)

add_library(
        ${PROJECT_NAME}_shared SHARED
        ${DWARF_PP_SRC}
        ${DWARF_PP_HEADERS}
)

set_target_properties(${PROJECT_NAME}_static
        PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
        )

set_target_properties(${PROJECT_NAME}_shared
        PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
        )

target_include_directories(${PROJECT_NAME}_static
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        )

target_include_directories(${PROJECT_NAME}_shared
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        )