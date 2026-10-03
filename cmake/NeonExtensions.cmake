# Native extensions: libraries an application loads when it starts, from the
# folder `extensions` next to it. See docs/extensions.md.

# This file is also what an extension that is built without the engine
# includes, through NeonSdk.cmake: it needs nothing of the engine's build,
# and names the header an extension is built against itself.
if (NOT TARGET neon-extension-api)
  add_library(neon-extension-api INTERFACE)
  target_include_directories(neon-extension-api
          INTERFACE "${CMAKE_CURRENT_LIST_DIR}/../lib/neon-extension-api"
  )
endif ()

# What a platform is called where a library is named for it: in the recipe
# of an extension, and in the name of the library this build makes. The
# application is told the same name, so that the two cannot differ.
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(NEON_PLATFORM_SYSTEM "macos")
  set(NEON_EXTENSION_SUFFIX ".dylib")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(NEON_PLATFORM_SYSTEM "windows")
  set(NEON_EXTENSION_SUFFIX ".dll")
else ()
  set(NEON_PLATFORM_SYSTEM "linux")
  set(NEON_EXTENSION_SUFFIX ".so")
endif ()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64)$")
  set(NEON_PLATFORM_PROCESSOR "arm64")
else ()
  set(NEON_PLATFORM_PROCESSOR "x86_64")
endif ()

set(NEON_PLATFORM "${NEON_PLATFORM_SYSTEM}-${NEON_PLATFORM_PROCESSOR}")
message("platform of extensions: ${NEON_PLATFORM}")

# neon_add_extension(<name> SOURCES <file>... [RECIPE <extension.yml>] [ASSETS <folder>] [DIRECTORY <folder>] [EXCLUDE_FROM_ALL])
#
# Builds a native extension and puts it where an application finds it:
#
#   <folder>/<name>/extension.yml
#   <folder>/<name>/<name>-<platform>.dylib, .so, or .dll
#   <folder>/<name>/assets/    what the extension brings besides its code
#
# The target is called <name>-extension. It is built against the header of
# neon-extension-api alone and links nothing of the engine, and whatever it
# does not mark with NEON_EXTENSION_EXPORT stays hidden.
#
# RECIPE is the recipe of the extension, which is copied next to the
# library. Without it, it is extension.yml next to the CMakeLists.txt.
#
# ASSETS is the folder of what the extension brings besides its code: scenes,
# prefabs, scripts, user interfaces, anything it reads, reached as
# extensions://<name>/assets/. Without it, it is the folder `assets` next to
# the recipe, when there is one.
#
# DIRECTORY is the folder `extensions` of an application. Without it, it is
# that of NeonRuntime, NEON_EXTENSIONS_DIRECTORY.
function(neon_add_extension NAME)
  cmake_parse_arguments(PARSE_ARGV 1 EXTENSION "EXCLUDE_FROM_ALL" "RECIPE;ASSETS;DIRECTORY" "SOURCES")

  if (NOT EXTENSION_RECIPE)
    set(EXTENSION_RECIPE "${CMAKE_CURRENT_SOURCE_DIR}/extension.yml")
  endif ()
  if (NOT EXTENSION_DIRECTORY)
    set(EXTENSION_DIRECTORY "${NEON_EXTENSIONS_DIRECTORY}")
  endif ()

  if (NOT EXTENSION_ASSETS)
    get_filename_component(RECIPE_FOLDER "${EXTENSION_RECIPE}" DIRECTORY)
    if (IS_DIRECTORY "${RECIPE_FOLDER}/assets")
      set(EXTENSION_ASSETS "${RECIPE_FOLDER}/assets")
    endif ()
  endif ()

  set(EXTENSION_TARGET "${NAME}-extension")
  set(EXTENSION_FOLDER "${EXTENSION_DIRECTORY}/${NAME}")

  if (EXTENSION_EXCLUDE_FROM_ALL)
    add_library(${EXTENSION_TARGET} MODULE EXCLUDE_FROM_ALL ${EXTENSION_SOURCES})
  else ()
    add_library(${EXTENSION_TARGET} MODULE ${EXTENSION_SOURCES})
  endif ()
  # the warnings of the engine, where the engine is what builds it
  if (COMMAND neon_warnings)
    neon_warnings(${EXTENSION_TARGET})
  endif ()

  target_link_libraries(${EXTENSION_TARGET} PRIVATE neon-extension-api)

  # The name is the same on every platform but for its end, so that a recipe
  # is written without looking at what a platform would have called it, and
  # the libraries of several platforms lie in one folder.
  set_target_properties(${EXTENSION_TARGET} PROPERTIES
          PREFIX ""
          OUTPUT_NAME "${NAME}-${NEON_PLATFORM}"
          SUFFIX "${NEON_EXTENSION_SUFFIX}"
          # a debug build is named as any other, since the recipe names it
          DEBUG_POSTFIX ""
          LIBRARY_OUTPUT_DIRECTORY "${EXTENSION_FOLDER}"
          RUNTIME_OUTPUT_DIRECTORY "${EXTENSION_FOLDER}"
          C_VISIBILITY_PRESET hidden
          CXX_VISIBILITY_PRESET hidden
          VISIBILITY_INLINES_HIDDEN ON
  )

  # An extension brings its own C++ runtime, as the applications do, so that
  # it runs where none is installed. Apple has no static one to link. With
  # llvm-mingw the flag for the C++ runtime alone leaves libunwind.dll to be
  # found when the extension is opened, so everything is linked statically
  # there, as the toolchain file does for the executables.
  if (MINGW)
    target_link_options(${EXTENSION_TARGET} PRIVATE -static)
  elseif (NOT APPLE)
    # The static runtime is not built hidden, and would be exported with the
    # extension, hundreds of names next to the one it marks.
    target_link_options(${EXTENSION_TARGET} PRIVATE -static-libstdc++ -Wl,--exclude-libs,ALL)
  endif ()

  add_custom_command(TARGET ${EXTENSION_TARGET} POST_BUILD
          COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${EXTENSION_RECIPE}" "${EXTENSION_FOLDER}/extension.yml"
          COMMENT "Copying the recipe of the extension ${NAME}"
  )

  # The assets are copied whenever the extension is built, not only when its
  # library is linked again, so that a scene that was edited is the scene
  # that runs. What the source no longer has stays in the copy until the
  # folder is removed.
  if (EXTENSION_ASSETS)
    add_custom_target(${EXTENSION_TARGET}-assets
            COMMENT "Copying the assets of the extension ${NAME}"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${EXTENSION_ASSETS}" "${EXTENSION_FOLDER}/assets"
            VERBATIM
    )
    add_dependencies(${EXTENSION_TARGET} ${EXTENSION_TARGET}-assets)
  endif ()
endfunction()
