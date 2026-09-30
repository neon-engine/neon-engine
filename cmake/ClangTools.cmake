# Targets that check the code against the style guide, docs/style-guide.md.
#
#   tidy          runs clang-tidy over the sources of the given targets
#   format-check  lists what clang-format would change, and changes nothing
#
# Both use the rules in .clang-tidy and .clang-format at the repository root.
# Neither is part of a normal build.
#
# Usage, after the targets of the project exist:
#   setup_clang_tools(neon-core neon-vulkan neon-sdl2 NeonRuntime)

option(NEON_CLANG_TIDY "Run clang-tidy on the engine's own sources in every build" OFF)
option(NEON_TIDY_WARNINGS_AS_ERRORS "Let the tidy target fail on any finding" OFF)

# The tools have to come from the same LLVM as the compiler. Homebrew keeps
# LLVM out of PATH on macOS, so the folder of the compiler and the folders
# Homebrew installs to are searched before PATH.
get_filename_component(NEON_COMPILER_DIRECTORY "${CMAKE_CXX_COMPILER}" DIRECTORY)
set(NEON_LLVM_HINTS
        "${NEON_COMPILER_DIRECTORY}"
        /opt/homebrew/opt/llvm@20/bin
        /usr/local/opt/llvm@20/bin
        /opt/homebrew/opt/llvm/bin
        /usr/local/opt/llvm/bin
)

find_program(NEON_CLANG_TIDY_EXECUTABLE NAMES clang-tidy clang-tidy-20 HINTS ${NEON_LLVM_HINTS})
find_program(NEON_CLANG_FORMAT_EXECUTABLE NAMES clang-format clang-format-20 HINTS ${NEON_LLVM_HINTS})

# Adds a target that only explains which tool is missing, and fails.
function(add_missing_tool_target NAME TOOL)
  add_custom_target(${NAME}
          COMMAND ${CMAKE_COMMAND} -E echo
          "${TOOL} was not found. Install LLVM 20, see docs/development.md, then run the configure step again."
          COMMAND ${CMAKE_COMMAND} -E false
          VERBATIM
  )
endfunction()

function(setup_clang_tools)
  set(CHECKED_SOURCES "")
  set(CHECKED_FILES "")

  set(TIDY_EXTRA_ARGUMENTS "")
  if (APPLE)
    # The compiler of Homebrew knows where the SDK of macOS is, clang-tidy
    # does not and has to be told
    set(MACOS_SDK "${CMAKE_OSX_SYSROOT}")
    if (NOT MACOS_SDK)
      execute_process(
              COMMAND xcrun --show-sdk-path
              OUTPUT_VARIABLE MACOS_SDK
              OUTPUT_STRIP_TRAILING_WHITESPACE
              ERROR_QUIET
      )
    endif ()
    if (MACOS_SDK)
      list(APPEND TIDY_EXTRA_ARGUMENTS "--extra-arg=-isysroot${MACOS_SDK}")
    endif ()
  endif ()

  foreach (CHECKED_TARGET IN LISTS ARGN)
    get_target_property(TARGET_SOURCES ${CHECKED_TARGET} SOURCES)
    get_target_property(TARGET_DIRECTORY ${CHECKED_TARGET} SOURCE_DIR)

    foreach (TARGET_SOURCE IN LISTS TARGET_SOURCES)
      get_filename_component(TARGET_SOURCE "${TARGET_SOURCE}" ABSOLUTE BASE_DIR "${TARGET_DIRECTORY}")
      list(APPEND CHECKED_FILES "${TARGET_SOURCE}")
      if (TARGET_SOURCE MATCHES "\\.cpp$")
        list(APPEND CHECKED_SOURCES "${TARGET_SOURCE}")
      endif ()
    endforeach ()

    if (NEON_CLANG_TIDY)
      if (NOT NEON_CLANG_TIDY_EXECUTABLE)
        message(FATAL_ERROR "NEON_CLANG_TIDY is on, but clang-tidy was not found. Install LLVM 20, see docs/development.md")
      endif ()
      set_target_properties(${CHECKED_TARGET} PROPERTIES
              CXX_CLANG_TIDY "${NEON_CLANG_TIDY_EXECUTABLE};${TIDY_EXTRA_ARGUMENTS}")
    endif ()
  endforeach ()

  list(REMOVE_DUPLICATES CHECKED_SOURCES)
  list(REMOVE_DUPLICATES CHECKED_FILES)

  if (NEON_CLANG_TIDY_EXECUTABLE)
    message("clang-tidy: ${NEON_CLANG_TIDY_EXECUTABLE}")

    # The top-level CMakeLists.txt points CMAKE_BINARY_DIR somewhere else, the
    # folder of the project is where compile_commands.json is written
    set(TIDY_ARGUMENTS -p "${PROJECT_BINARY_DIR}" --quiet ${TIDY_EXTRA_ARGUMENTS})
    if (NEON_TIDY_WARNINGS_AS_ERRORS)
      list(APPEND TIDY_ARGUMENTS "--warnings-as-errors=*")
    endif ()

    # One target for each source file, so that the build tool checks several
    # files at the same time.
    add_custom_target(tidy)
    foreach (CHECKED_SOURCE IN LISTS CHECKED_SOURCES)
      file(RELATIVE_PATH SOURCE_NAME "${CMAKE_SOURCE_DIR}" "${CHECKED_SOURCE}")
      string(REGEX REPLACE "[^A-Za-z0-9]" "-" SOURCE_TARGET "tidy-${SOURCE_NAME}")

      add_custom_target(${SOURCE_TARGET}
              COMMAND "${NEON_CLANG_TIDY_EXECUTABLE}" ${TIDY_ARGUMENTS} "${CHECKED_SOURCE}"
              WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
              COMMENT "clang-tidy ${SOURCE_NAME}"
              VERBATIM
      )
      add_dependencies(tidy ${SOURCE_TARGET})
    endforeach ()
  else ()
    message("clang-tidy: not found, the tidy target will say so")
    add_missing_tool_target(tidy clang-tidy)
  endif ()

  if (NEON_CLANG_FORMAT_EXECUTABLE)
    message("clang-format: ${NEON_CLANG_FORMAT_EXECUTABLE}")

    add_custom_target(format-check
            COMMAND "${NEON_CLANG_FORMAT_EXECUTABLE}" --dry-run --style=file ${CHECKED_FILES}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            COMMENT "clang-format, nothing is changed"
            VERBATIM
    )
  else ()
    message("clang-format: not found, the format-check target will say so")
    add_missing_tool_target(format-check clang-format)
  endif ()
endfunction()
