# Projects with code of their own: a game in C++ next to the engine, put
# together from a runtime, the project's assets, and its extension. See
# docs/projects.md and docs/extensions.md.

# The runtime a project is put together with when the engine is not what
# builds it: a folder with NeonRuntime and its engine folder, as the engine's build
# or a release of the engine leaves it. Looked for among the builds of the
# engine this file belongs to, the release first.
if (NOT TARGET NeonRuntime AND NOT NEON_RUNTIME_DIRECTORY)
  string(TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" NEON_RUNTIME_PLATFORM)
  foreach (BUILD IN ITEMS release debug)
    set(CANDIDATE "${CMAKE_CURRENT_LIST_DIR}/../bin/${BUILD}/${NEON_RUNTIME_PLATFORM}/NeonRuntime")
    if (NOT NEON_RUNTIME_DIRECTORY AND EXISTS "${CANDIDATE}/engine")
      get_filename_component(NEON_RUNTIME_DIRECTORY "${CANDIDATE}" ABSOLUTE)
    endif ()
  endforeach ()
endif ()
set(NEON_RUNTIME_DIRECTORY "${NEON_RUNTIME_DIRECTORY}" CACHE PATH
        "The folder of a NeonRuntime that is built already, with its engine folder, for projects built without the engine")

# neon_add_project(<name> [SOURCES <file>...] [FOLDERS <folder>...] [DIRECTORY <folder>])
#
# Declares the project in the folder of the CMakeLists.txt that calls it:
#
#   assets/                      the project: project.yml, and what it has
#                                of scenes, scripts, and the rest
#   extensions/<name>/           its extension, laid out as it is next to the
#                                runtime:
#     extension.yml              the recipe
#     ...                        SOURCES, the code, and FOLDERS, what the
#                                extension needs to run besides it, such as
#                                `scripts`, see neon_add_extension
#
# What the game shows and reads, its scenes, prefabs, user interfaces, and
# scripts, is in assets/, whether or not the extension brings what they use.
# A project need not have an extension, and then names no SOURCES. An
# extension that belongs to no project is declared with neon_add_extension
# alone.
#
# The target <name> puts the game together:
#
#   <name>/NeonRuntime           the runtime, as it was built
#   <name>/engine/               what the runtime ships with, engine://: its
#                                shaders and fonts
#   <name>/assets/               the project's assets, assets://
#   <name>/extensions/<name>/    the extension, see neon_add_extension
#
# Nothing of a game comes from the runtime: it is a runtime and knows none.
# The project brings everything it shows and reads, project.yml, its menus,
# and its input map among them.
#
# DIRECTORY is where the game is put together, when it is not where the two
# ways below say.
#
# There are two ways to build it, and the CMakeLists.txt is the same for both.
#
# On its own, which is how a game is worked on: the folder of the project is
# configured as a build of its own that includes NeonSdk.cmake. Only the
# extension is compiled, and the runtime is the one in NEON_RUNTIME_DIRECTORY.
# The game is put together in the build folder, and the target is part of
# the default build.
#
# By the engine's build, for a project in projects/ of the engine: the
# runtime is the one the engine builds, the game is put together under bin/
# next to the applications, and the target is not part of the default
# build: the build presets name it, so `cmake --build --preset <preset>`
# builds it, and so does `cmake --build <build> --target <name>`.
function(neon_add_project NAME)
  cmake_parse_arguments(PARSE_ARGV 1 PROJECT "" "DIRECTORY" "SOURCES;FOLDERS")
  set(PROJECT_DIRECTORY_GIVEN "${PROJECT_DIRECTORY}")

  if (TARGET NeonRuntime)
    set(BUILT_BY_THE_ENGINE ON)
    set(PROJECT_DIRECTORY "${BASE_APP_BIN_DIRECTORY}/${NAME}")
    set(RUNTIME_DIRECTORY "${BASE_APP_BIN_DIRECTORY}/NeonRuntime")
    set(RUNTIME_FILE "$<TARGET_FILE:NeonRuntime>")
    set(IN_DEFAULT_BUILD "")
  else ()
    set(BUILT_BY_THE_ENGINE OFF)
    if (NOT NEON_RUNTIME_DIRECTORY OR NOT EXISTS "${NEON_RUNTIME_DIRECTORY}/engine")
      message(FATAL_ERROR
              "The project ${NAME} needs a NeonRuntime that is built already. Build the engine once, or say where "
              "one is with -DNEON_RUNTIME_DIRECTORY=<folder with NeonRuntime and its engine folder>")
    endif ()
    set(PROJECT_DIRECTORY "${CMAKE_BINARY_DIR}/${NAME}")
    set(RUNTIME_DIRECTORY "${NEON_RUNTIME_DIRECTORY}")
    set(RUNTIME_FILE "${NEON_RUNTIME_DIRECTORY}/NeonRuntime${CMAKE_EXECUTABLE_SUFFIX}")
    set(IN_DEFAULT_BUILD ALL)
    message("the project ${NAME} is put together in ${PROJECT_DIRECTORY} with the runtime of ${RUNTIME_DIRECTORY}")
  endif ()

  # somewhere else when asked, as a test puts its game where the tests are
  if (PROJECT_DIRECTORY_GIVEN)
    set(PROJECT_DIRECTORY "${PROJECT_DIRECTORY_GIVEN}")
  endif ()

  if (PROJECT_SOURCES)
    neon_add_extension(${NAME}
            SOURCES ${PROJECT_SOURCES}
            RECIPE "${CMAKE_CURRENT_SOURCE_DIR}/extensions/${NAME}/extension.yml"
            FOLDERS ${PROJECT_FOLDERS}
            DIRECTORY "${PROJECT_DIRECTORY}/extensions"
            EXCLUDE_FROM_ALL
    )
  endif ()

  add_custom_target(${NAME} ${IN_DEFAULT_BUILD}
          COMMENT "Putting the project ${NAME} together in ${PROJECT_DIRECTORY}"
          COMMAND "${CMAKE_COMMAND}" -E copy_directory "${RUNTIME_DIRECTORY}/engine" "${PROJECT_DIRECTORY}/engine"
          COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${PROJECT_DIRECTORY}/assets"
          COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${RUNTIME_FILE}" "${PROJECT_DIRECTORY}/"
          VERBATIM
  )

  # the runtime with its assets copied and its shaders compiled, where the
  # engine builds it, and the extension, before any of it is put together
  if (BUILT_BY_THE_ENGINE)
    add_dependencies(${NAME} NeonRuntime NeonRuntime_copy_engine NeonRuntime_compile_shaders)
  endif ()
  if (PROJECT_SOURCES)
    add_dependencies(${NAME} ${NAME}-extension)
  endif ()

  # where it is put together, for what starts it, as a test does
  set_target_properties(${NAME} PROPERTIES NEON_PROJECT_DIRECTORY "${PROJECT_DIRECTORY}")
endfunction()
