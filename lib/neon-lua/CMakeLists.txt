# Lua itself, from external/lua, which ships no build of its own. The
# interpreter (lua.c) and the one-file build (onelua.c) are left out, as is
# ltests.c, which is the test harness of the Lua authors.
set(LUA_SOURCES
        lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c ldblib.c ldebug.c ldo.c
        ldump.c lfunc.c lgc.c linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c
        lopcodes.c loslib.c lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltm.c
        lundump.c lutf8lib.c lvm.c lzio.c
)
list(TRANSFORM LUA_SOURCES PREPEND "${CMAKE_SOURCE_DIR}/external/lua/")

add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua SYSTEM PUBLIC "${CMAKE_SOURCE_DIR}/external/lua")

# Lua is C; the engine's warnings are for its own C++ and stop at it
if (APPLE)
  target_compile_definitions(lua PRIVATE LUA_USE_MACOSX)
elseif (UNIX)
  target_compile_definitions(lua PRIVATE LUA_USE_LINUX)
endif ()

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
        neon/scripting/lua-entity-handle.cpp
        neon/scripting/lua-entity-handle.hpp
        neon/scripting/lua-host.cpp
        neon/scripting/lua-host.hpp
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

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/scripting/lua-script-system.test.cpp)
endif ()
