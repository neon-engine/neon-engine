# setup_copy_assets(<target> <source folder> <output folder> [PART <name>] [EXCLUDE <regex>])
#
# Copies a folder next to the target whenever it is built, as the folder
# `assets` of a game or `engine` of the runtime. The copy is the target
# <target>_copy_<name>, `assets` unless PART names another, so that one target
# can have several folders copied. Files whose path matches EXCLUDE are left
# out, as the sources of shaders, which are compiled in place of a copy.
function(setup_copy_assets TARGET_NAME SOURCE_ASSETS_DIR OUTPUT_ASSETS_DIR)
    cmake_parse_arguments(PARSE_ARGV 3 COPY "" "PART;EXCLUDE" "")
    if (NOT COPY_PART)
        set(COPY_PART assets)
    endif ()
    set(COPY_TARGET ${TARGET_NAME}_copy_${COPY_PART})

    add_custom_target(${COPY_TARGET} ALL
            COMMENT "Syncing ${COPY_PART} for ${TARGET_NAME}"
    )

    # The copy below only adds and replaces, so this first removes what the
    # source no longer has, files and the folders they leave empty, and the
    # destination mirrors the source. Removing only that keeps an incremental
    # build fast, see RemoveOrphans.cmake (#177).
    add_custom_command(
            TARGET ${COPY_TARGET}
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND}
            "-DSOURCE_ASSETS_DIR=${SOURCE_ASSETS_DIR}"
            "-DOUTPUT_ASSETS_DIR=${OUTPUT_ASSETS_DIR}"
            -P "${CMAKE_SOURCE_DIR}/cmake/scripts/RemoveOrphans.cmake"
            VERBATIM
    )

    # CONFIGURE_DEPENDS makes a build look for assets that were added or
    # removed since the build was configured, and configure again when there
    # are any, so that a new scene is copied without configuring by hand.
    file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
            "${SOURCE_ASSETS_DIR}/*"
    )
    list(FILTER SOURCE_FILES EXCLUDE REGEX "\\.gitignore$")
    if (COPY_EXCLUDE)
        list(FILTER SOURCE_FILES EXCLUDE REGEX "${COPY_EXCLUDE}")
    endif ()
    # what macOS leaves in folders, which no game reads: Finder's settings,
    # and the icon of a folder, whose name ends in a carriage return that
    # breaks the commands of the build
    list(FILTER SOURCE_FILES EXCLUDE REGEX "/(\\.DS_Store|Icon\r)$")

    foreach (SRC_FILE IN LISTS SOURCE_FILES)
        file(RELATIVE_PATH REL_PATH "${SOURCE_ASSETS_DIR}" "${SRC_FILE}")
        get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
        set(DEST_FILE "${OUTPUT_ASSETS_DIR}/${REL_PATH}")

        add_custom_command(
                TARGET ${COPY_TARGET}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "${OUTPUT_ASSETS_DIR}/${REL_DIR}"

                COMMAND ${CMAKE_COMMAND}
                "-DSRC=${SRC_FILE}"
                "-DDST=${DEST_FILE}"
                -P "${CMAKE_SOURCE_DIR}/cmake/scripts/CheckAndCopy.cmake"

                DEPENDS "${SRC_FILE}"

                # every argument reaches the command as it is, so that a
                # name with spaces or parentheses is one argument
                VERBATIM
        )
    endforeach ()

    add_dependencies(${TARGET_NAME} ${COPY_TARGET})
endfunction()
