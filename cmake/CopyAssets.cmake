function(setup_copy_assets TARGET_NAME SOURCE_ASSETS_DIR OUTPUT_ASSETS_DIR)
    add_custom_target(${TARGET_NAME}_copy_assets ALL
            COMMENT "Syncing assets for ${TARGET_NAME}"
    )

    add_custom_command(
            TARGET ${TARGET_NAME}_copy_assets
            PRE_BUILD
            COMMAND ${CMAKE_COMMAND}
            -DSOURCE_ASSETS_DIR="${SOURCE_ASSETS_DIR}"
            -DOUTPUT_ASSETS_DIR="${OUTPUT_ASSETS_DIR}"
            -P "${CMAKE_SOURCE_DIR}/cmake/scripts/RemoveOrphans.cmake"
    )

    file(GLOB_RECURSE SOURCE_FILES
            "${SOURCE_ASSETS_DIR}/*"
    )
    list(FILTER SOURCE_FILES EXCLUDE REGEX "\\.dvc$")
    list(FILTER SOURCE_FILES EXCLUDE REGEX "\\.gitignore$")

    foreach (SRC_FILE IN LISTS SOURCE_FILES)
        file(RELATIVE_PATH REL_PATH "${SOURCE_ASSETS_DIR}" "${SRC_FILE}")
        get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
        set(DEST_FILE "${OUTPUT_ASSETS_DIR}/${REL_PATH}")

        add_custom_command(
                TARGET ${TARGET_NAME}_copy_assets
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "${OUTPUT_ASSETS_DIR}/${REL_DIR}"

                COMMAND ${CMAKE_COMMAND}
                -DSRC="${SRC_FILE}"
                -DDST="${DEST_FILE}"
                -P "${CMAKE_SOURCE_DIR}/cmake/scripts/CheckAndCopy.cmake"

                DEPENDS "${SRC_FILE}"
        )
    endforeach ()

    add_dependencies(${TARGET_NAME} ${TARGET_NAME}_copy_assets)
endfunction()
