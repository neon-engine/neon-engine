# Compiles the shader sources of the Vulkan backend into SPIR-V.
#
# Every *.vert and *.frag file in SOURCE_SHADERS_DIR becomes
# <OUTPUT_SHADERS_DIR>/<file name>.spv, so basic-lit.vert turns into
# basic-lit.vert.spv. A material names a shader without an extension, such as
# engine://shaders/basic-lit, and the Vulkan backend adds .vert.spv and
# .frag.spv to it.
#
# FOLDERS names folders inside SOURCE_SHADERS_DIR that are compiled as well,
# each into a folder of that name: ui/shine.frag turns into
# <OUTPUT_SHADERS_DIR>/ui/shine.frag.spv. What is in a folder includes the
# shared declarations of the folder above it, as "../ui-shader.glsl".
function(setup_compile_shaders TARGET_NAME SOURCE_SHADERS_DIR OUTPUT_SHADERS_DIR)
    cmake_parse_arguments(PARSE_ARGV 3 SHADERS "" "" "FOLDERS")

    find_program(GLSLANG_EXECUTABLE NAMES glslang glslangValidator)
    if (NOT GLSLANG_EXECUTABLE)
        message(FATAL_ERROR
                "glslang was not found. It compiles the shaders of the Vulkan backend.\n"
                "  macOS:  brew install glslang\n"
                "  Ubuntu: apt install glslang-tools")
    endif ()

    file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS
            "${SOURCE_SHADERS_DIR}/*.vert"
            "${SOURCE_SHADERS_DIR}/*.frag"
    )

    # shared declarations that the sources pull in with #include
    file(GLOB SHADER_INCLUDES CONFIGURE_DEPENDS
            "${SOURCE_SHADERS_DIR}/*.glsl"
    )

    set(SHADER_OUTPUTS)
    foreach (SHADER_SOURCE IN LISTS SHADER_SOURCES)
        get_filename_component(SHADER_NAME "${SHADER_SOURCE}" NAME)
        set(SHADER_OUTPUT "${OUTPUT_SHADERS_DIR}/${SHADER_NAME}.spv")

        add_custom_command(
                OUTPUT "${SHADER_OUTPUT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_SHADERS_DIR}"
                COMMAND "${GLSLANG_EXECUTABLE}" -V --quiet "${SHADER_SOURCE}" -o "${SHADER_OUTPUT}"
                DEPENDS "${SHADER_SOURCE}" ${SHADER_INCLUDES}
                COMMENT "Compiling shader ${SHADER_NAME}"
                VERBATIM
        )
        list(APPEND SHADER_OUTPUTS "${SHADER_OUTPUT}")
    endforeach ()

    foreach (FOLDER IN LISTS SHADERS_FOLDERS)
        file(GLOB FOLDER_SOURCES CONFIGURE_DEPENDS
                "${SOURCE_SHADERS_DIR}/${FOLDER}/*.vert"
                "${SOURCE_SHADERS_DIR}/${FOLDER}/*.frag"
        )

        file(GLOB FOLDER_INCLUDES CONFIGURE_DEPENDS
                "${SOURCE_SHADERS_DIR}/${FOLDER}/*.glsl"
        )

        foreach (SHADER_SOURCE IN LISTS FOLDER_SOURCES)
            get_filename_component(SHADER_NAME "${SHADER_SOURCE}" NAME)
            set(SHADER_OUTPUT "${OUTPUT_SHADERS_DIR}/${FOLDER}/${SHADER_NAME}.spv")

            add_custom_command(
                    OUTPUT "${SHADER_OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_SHADERS_DIR}/${FOLDER}"
                    COMMAND "${GLSLANG_EXECUTABLE}" -V --quiet "${SHADER_SOURCE}" -o "${SHADER_OUTPUT}"
                    DEPENDS "${SHADER_SOURCE}" ${SHADER_INCLUDES} ${FOLDER_INCLUDES}
                    COMMENT "Compiling shader ${FOLDER}/${SHADER_NAME}"
                    VERBATIM
            )
            list(APPEND SHADER_OUTPUTS "${SHADER_OUTPUT}")
        endforeach ()
    endforeach ()

    add_custom_target(${TARGET_NAME}_compile_shaders ALL DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_compile_shaders)

    # A copy of a folder removes empty folders the source lacks, and the
    # shaders' folder is one until a shader is written into it: so the
    # shaders are compiled after every copy, never beside it.
    foreach (COPY_TARGET IN ITEMS ${TARGET_NAME}_copy_assets ${TARGET_NAME}_copy_engine)
        if (TARGET ${COPY_TARGET})
            add_dependencies(${TARGET_NAME}_compile_shaders ${COPY_TARGET})
        endif ()
    endforeach ()
endfunction()
