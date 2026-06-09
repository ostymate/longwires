file(GLOB_RECURSE SOURCES "core/*.c" "devices/*.c" "platforms/*.c")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        "core"
        "devices"
        "platforms"
    REQUIRES
        driver
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE ESP_PLATFORM)