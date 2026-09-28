# Usage:
#   cmake -D SOURCE_ASSETS_DIR="..." -D OUTPUT_ASSETS_DIR="..." -P RemoveOrphans.cmake

if (NOT DEFINED SOURCE_ASSETS_DIR)
    message(FATAL_ERROR "SOURCE_ASSETS_DIR variable not defined!")
endif ()
if (NOT DEFINED OUTPUT_ASSETS_DIR)
    message(FATAL_ERROR "OUTPUT_ASSETS_DIR variable not defined!")
endif ()

file(GLOB_RECURSE CURRENT_TARGET_FILES
        "${OUTPUT_ASSETS_DIR}/*"
)

# compiled shaders are produced by the build, they have no counterpart in the
# source assets and must survive
list(FILTER CURRENT_TARGET_FILES EXCLUDE REGEX "\\.spv$")

foreach (TGT_FILE IN LISTS CURRENT_TARGET_FILES)
    file(RELATIVE_PATH TGT_FILE_REL "${OUTPUT_ASSETS_DIR}" "${TGT_FILE}")

    set(SRC_FILE "${SOURCE_ASSETS_DIR}/${TGT_FILE_REL}")
    if (NOT EXISTS "${SRC_FILE}")
        message(STATUS "Removing orphaned file: ${TGT_FILE}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E remove "${TGT_FILE}")
    endif ()
endforeach ()
