# Usage:
#   cmake -D SOURCE_ASSETS_DIR="..." -D OUTPUT_ASSETS_DIR="..." -P RemoveOrphans.cmake
#
# Makes the copied assets mirror the source: whatever the destination holds
# that the source does not is removed, the files first and then the folders
# they leave empty. Without this a renamed or removed asset lingers next to
# the binary, a scene that still names it runs on the machine that renamed it
# and fails on a fresh checkout, and a tool that walks assets:// sees files the
# project no longer has (#177). Only what is missing goes, which keeps an
# incremental build fast; emptying the folder and copying it whole would copy
# every asset on every build.

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
    # what macOS leaves in folders is never copied, see CopyAssets.cmake, so
    # it is an orphan even when the source has one too
    if (NOT EXISTS "${SRC_FILE}" OR TGT_FILE_REL MATCHES "(^|/)(\\.DS_Store|Icon\r)$")
        message(STATUS "Removing orphaned file: ${TGT_FILE}")
        file(REMOVE "${TGT_FILE}")
    endif ()
endforeach ()

# Folders the source lacks go once they are empty, deepest first, so that a
# folder whose only content was another orphaned folder goes too. Sorting the
# paths in descending order puts every folder before the one that holds it. A
# folder that still holds something, such as the compiled shaders, stays.
file(GLOB_RECURSE CURRENT_TARGET_ENTRIES LIST_DIRECTORIES true
        "${OUTPUT_ASSETS_DIR}/*"
)
set(CURRENT_TARGET_DIRS)
foreach (TGT_ENTRY IN LISTS CURRENT_TARGET_ENTRIES)
    if (IS_DIRECTORY "${TGT_ENTRY}")
        list(APPEND CURRENT_TARGET_DIRS "${TGT_ENTRY}")
    endif ()
endforeach ()
list(SORT CURRENT_TARGET_DIRS ORDER DESCENDING)

foreach (TGT_DIR IN LISTS CURRENT_TARGET_DIRS)
    file(RELATIVE_PATH TGT_DIR_REL "${OUTPUT_ASSETS_DIR}" "${TGT_DIR}")

    if (NOT IS_DIRECTORY "${SOURCE_ASSETS_DIR}/${TGT_DIR_REL}")
        file(GLOB LEFT_INSIDE "${TGT_DIR}/*")
        if (NOT LEFT_INSIDE)
            message(STATUS "Removing orphaned folder: ${TGT_DIR}")
            file(REMOVE_RECURSE "${TGT_DIR}")
        endif ()
    endif ()
endforeach ()
