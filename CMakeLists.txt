file(GLOB_RECURSE SOURCES "bb_core/*.c" "devices/*.c")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        "bb_core"
        "devices"
    REQUIRES
        driver
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE ESP_PLATFORM)