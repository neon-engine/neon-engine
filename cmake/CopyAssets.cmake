
function(setup_copy_assets TARGET_NAME SOURCE_ASSETS_DIR OUTPUT_ASSETS_DIR)
  # Set COMPILE_SHADERS to FALSE by default if not provided
  if (${ARGC} GREATER 3)
    set(COMPILE_SHADERS ${ARGV3})
  else ()
    set(COMPILE_SHADERS FALSE)
  endif ()

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
    get_filename_component(FILE_EXT "${SRC_FILE}" EXT)
    get_filename_component(FILE_NAME_ONLY "${SRC_FILE}" NAME_WE)
    set(DEST_FILE "${OUTPUT_ASSETS_DIR}/${REL_PATH}")

    # Check if this is a shader file that needs to be compiled
    if (COMPILE_SHADERS AND REL_DIR MATCHES "shaders")
      # Typical shader extensions
      if (FILE_EXT MATCHES "\\.(vert|frag|comp|geom|tesc|tese|glsl)$")
        # Create a better output name for the SPV file (without duplicating the extension)
        set(SPV_DEST_FILE "${OUTPUT_ASSETS_DIR}/${REL_DIR}/${FILE_NAME_ONLY}${FILE_EXT}.spv")

        add_custom_command(
                TARGET ${TARGET_NAME}_copy_assets
                COMMAND ${CMAKE_COMMAND} -E make_directory
                "${OUTPUT_ASSETS_DIR}/${REL_DIR}"

                # Compile shader to SPIR-V using glslc
                COMMAND glslc "${SRC_FILE}" -o "${SPV_DEST_FILE}"

                DEPENDS "${SRC_FILE}"
                COMMENT "Compiling shader: ${REL_PATH} to SPIR-V"
        )
        # We don't add a command to copy the original shader file here
      else ()
        # For non-shader files in the shaders directory, just copy them
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
      endif ()
    else ()
      # For non-shader files or when shader compilation is disabled, copy normally
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
    endif ()
  endforeach ()

  add_dependencies(${TARGET_NAME} ${TARGET_NAME}_copy_assets)
endfunction()
