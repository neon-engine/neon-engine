# The Lua that runs the scripts is LuaJIT, the target `lua`: the language of
# Lua 5.1 with the parts of 5.2 it chose to add, see docs/scripting.md.
# LuaJIT is built by its own Makefile, which needs make and, when
# cross-compiling, a C compiler for the machine that builds, since two tools
# of the build run there. It is ported to x86, x64, ARM, and ARM64; another
# processor has no Lua to run the scripts and is refused here. On a platform
# that forbids writing code at run time, iOS and the consoles, LuaJIT turns
# its compiler off itself and is an interpreter; their SDK flags are not
# wired here yet.
if (NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64|ARM64|x86_64|AMD64|i[3-6]86|x86|arm|armv[6-8].*)$")
  message(FATAL_ERROR "LuaJIT, which runs the scripts, is not ported to ${CMAKE_SYSTEM_PROCESSOR}: "
          "it runs on x86, x64, ARM, and ARM64")
endif ()

# LuaJIT builds in its own tree with its own Makefile, so a copy of it is
# built, under the build folder, and the submodule stays clean.
find_program(NEON_MAKE NAMES gmake make REQUIRED)
set(LUAJIT_SOURCE "${CMAKE_SOURCE_DIR}/external/luajit")
set(LUAJIT_BUILD "${CMAKE_BINARY_DIR}/luajit")
set(LUAJIT_LIBRARY "${LUAJIT_BUILD}/src/libluajit.a")
file(GLOB LUAJIT_FILES CONFIGURE_DEPENDS
        "${LUAJIT_SOURCE}/src/*.c" "${LUAJIT_SOURCE}/src/*.h" "${LUAJIT_SOURCE}/src/*.dasc"
        "${LUAJIT_SOURCE}/src/Makefile" "${LUAJIT_SOURCE}/src/host/*" "${LUAJIT_SOURCE}/dynasm/*")

# the language of 5.2 where LuaJIT offers it: goto, table.pack,
# table.unpack, %q, and __pairs and __len on tables
set(LUAJIT_MAKE_ARGS BUILDMODE=static "XCFLAGS=-DLUAJIT_ENABLE_LUA52COMPAT" Q=)
set(LUAJIT_ENVIRONMENT)

if (CMAKE_CROSSCOMPILING)
  # The Makefile builds the library with CROSS + CC, the compiler of the
  # toolchain split into its folder and prefix, x86_64-w64-mingw32-, and
  # clang, so that it finds the ar and strip beside it; and two tools
  # that run on this machine with HOST_CC. That is the same clang, the
  # plain one beside the prefixed one, which builds for the machine it
  # runs on when told to link with lld; the C headers and the runtime
  # files of gcc have to be installed for that. NEON_HOST_CC overrides it.
  get_filename_component(LUAJIT_TOOLCHAIN_DIRECTORY "${CMAKE_C_COMPILER}" DIRECTORY)
  get_filename_component(LUAJIT_TOOLCHAIN_COMPILER "${CMAKE_C_COMPILER}" NAME)
  if (NOT NEON_HOST_CC)
    if (EXISTS "${LUAJIT_TOOLCHAIN_DIRECTORY}/clang")
      set(NEON_HOST_CC "${LUAJIT_TOOLCHAIN_DIRECTORY}/clang -fuse-ld=lld")
    else ()
      find_program(NEON_HOST_CC_FOUND NAMES cc gcc clang)
      if (NOT NEON_HOST_CC_FOUND)
        message(FATAL_ERROR "LuaJIT's build needs a C compiler for this machine, since two of its tools run "
                "here: set NEON_HOST_CC")
      endif ()
      set(NEON_HOST_CC "${NEON_HOST_CC_FOUND}")
    endif ()
  endif ()
  message("lua: LuaJIT's tools of the build are compiled by ${NEON_HOST_CC}")
  string(REGEX MATCH "(clang|gcc|cc)(\\.exe)?$" LUAJIT_CC "${LUAJIT_TOOLCHAIN_COMPILER}")
  string(REGEX REPLACE "(clang|gcc|cc)(\\.exe)?$" "" LUAJIT_PREFIX "${LUAJIT_TOOLCHAIN_COMPILER}")
  list(APPEND LUAJIT_MAKE_ARGS "HOST_CC=${NEON_HOST_CC}" "CROSS=${LUAJIT_TOOLCHAIN_DIRECTORY}/${LUAJIT_PREFIX}" "CC=${LUAJIT_CC}")

  if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    list(APPEND LUAJIT_MAKE_ARGS TARGET_SYS=Windows)
    # the Makefile gives the tools -malign-double for this target, a flag
    # of x86 alone; a machine of another kind builds them without it
    if (NOT CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i[3-6]86")
      list(APPEND LUAJIT_MAKE_ARGS "HOST_XCFLAGS=-I. -DLUAJIT_OS=LUAJIT_OS_WINDOWS")
    endif ()
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND LUAJIT_MAKE_ARGS TARGET_SYS=Linux)
  elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    list(APPEND LUAJIT_MAKE_ARGS TARGET_SYS=Darwin)
  else ()
    message(FATAL_ERROR "LuaJIT is not wired for cross-compiling to ${CMAKE_SYSTEM_NAME} yet")
  endif ()
else ()
  list(APPEND LUAJIT_MAKE_ARGS "CC=${CMAKE_C_COMPILER}")
endif ()
if (APPLE)
  # the Makefile insists on one
  set(LUAJIT_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}")
  if (NOT LUAJIT_DEPLOYMENT_TARGET)
    set(LUAJIT_DEPLOYMENT_TARGET "11.0")
  endif ()
  set(LUAJIT_ENVIRONMENT "MACOSX_DEPLOYMENT_TARGET=${LUAJIT_DEPLOYMENT_TARGET}")
endif ()

add_custom_command(
        OUTPUT "${LUAJIT_LIBRARY}"
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${LUAJIT_BUILD}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${LUAJIT_SOURCE}" "${LUAJIT_BUILD}"
        COMMAND ${CMAKE_COMMAND} -E env ${LUAJIT_ENVIRONMENT}
                "${NEON_MAKE}" -C "${LUAJIT_BUILD}/src" libluajit.a ${LUAJIT_MAKE_ARGS}
        DEPENDS ${LUAJIT_FILES}
        COMMENT "Building LuaJIT"
        VERBATIM
)
add_custom_target(luajit-build DEPENDS "${LUAJIT_LIBRARY}")

# luajit.h, with the version, is written by the build into the copy
add_library(lua INTERFACE)
target_include_directories(lua SYSTEM INTERFACE "${LUAJIT_SOURCE}/src" "${LUAJIT_BUILD}/src")
target_link_libraries(lua INTERFACE "${LUAJIT_LIBRARY}")
message("lua: LuaJIT from external/luajit, built by make for ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_PROCESSOR}")

set(TARGET neon-lua)

add_library(${TARGET} STATIC
        neon/scripting/lua-api.hpp
        neon/scripting/lua-classes.cpp
        neon/scripting/lua-classes.hpp
        neon/scripting/lua-color-handle.cpp
        neon/scripting/lua-color-handle.hpp
        neon/scripting/lua-component-declaration.hpp
        neon/scripting/lua-component-handle.cpp
        neon/scripting/lua-component-handle.hpp
        neon/scripting/lua-compat.cpp
        neon/scripting/lua-compat.hpp
        neon/scripting/lua-entity-handle.cpp
        neon/scripting/lua-entity-handle.hpp
        neon/scripting/lua-host.cpp
        neon/scripting/lua-host.hpp
        neon/scripting/lua-list-handle.cpp
        neon/scripting/lua-list-handle.hpp
        neon/scripting/lua-libraries.cpp
        neon/scripting/lua-libraries.hpp
        neon/scripting/lua-matrix-handle.cpp
        neon/scripting/lua-matrix-handle.hpp
        neon/scripting/lua-quat-handle.cpp
        neon/scripting/lua-quat-handle.hpp
        neon/scripting/lua-sandbox.cpp
        neon/scripting/lua-sandbox.hpp
        neon/scripting/lua-script-system.cpp
        neon/scripting/lua-script-system.hpp
        neon/scripting/lua-system-declaration.hpp
        neon/scripting/lua-vec2-handle.cpp
        neon/scripting/lua-vec2-handle.hpp
        neon/scripting/lua-vec3-handle.cpp
        neon/scripting/lua-vec3-handle.hpp
        neon/scripting/lua-vec4-handle.cpp
        neon/scripting/lua-vec4-handle.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Lua is private. What includes lua-script-system.hpp sees no declaration of
# Lua and cannot call it.
target_link_libraries(${TARGET}
        PUBLIC neon-core
        PRIVATE lua
)
add_dependencies(${TARGET} luajit-build)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/scripting/lua-script-system.test.cpp)
endif ()
