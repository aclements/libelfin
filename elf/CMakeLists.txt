project(elf++)

add_custom_command(
        OUTPUT to_string.cc
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/gen_to_string.sh
        ${CMAKE_CURRENT_BINARY_DIR}/to_string.cc
        DEPENDS enum-print.py gen_to_string.sh
)

set(ELF_PP_SOURCES
        elf.cc
        mmap_loader.cc
        ${CMAKE_CURRENT_BINARY_DIR}/to_string.cc
        )

set(ELF_PP_HEADERS
        common.hh
        data.hh
        elf++.hh
        to_hex.hh
        )

add_library(
        ${PROJECT_NAME}_static STATIC
        ${ELF_PP_SOURCES}
        ${ELF_PP_HEADERS}
)

add_library(
        ${PROJECT_NAME}_shared SHARED
        ${ELF_PP_SOURCES}
        ${ELF_PP_HEADERS}
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