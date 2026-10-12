set(TARGET NeonRuntime)

# The composition root and the runtime's own objects: the Runtime it runs,
# and its choices on the shared start-up of neon-runtime. The backends are
# named by neon-runtime alone; neon-sdl2-main is what the platform starts
# the application by, and has to be linked into the executable itself.
add_executable(${TARGET} main.cpp
        neon-runtime.cpp
        neon-runtime.hpp
        runtime-hooks.cpp
        runtime-hooks.hpp)

target_link_libraries(${TARGET} PRIVATE neon-runtime neon-sdl2-main)

# What the runtime ships with whatever the game, engine:// next to it: the
# fonts text falls back to, and the shaders compiled below. The sources of
# the shaders are kept where they ship, engine/shaders, and only what they
# are compiled to is next to the runtime. It ships no game: assets:// is the
# project's, see projects/sandbox for the one the engine is tried out with
setup_copy_assets(
        ${TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/engine"
        "${APP_BIN_DIRECTORY}/engine"
        PART engine
        EXCLUDE "\\.(vert|frag|glsl)$"
)

# A build folder from before kept the runtime's own project in assets/ next
# to it, which a runtime started there would still run. It goes, at every
# build, since the copy above is run at every build.
add_custom_command(TARGET ${TARGET}_copy_engine PRE_BUILD
        COMMAND "${CMAKE_COMMAND}" -E rm -rf "${APP_BIN_DIRECTORY}/assets"
        VERBATIM
)
neon_warnings(${TARGET})

setup_compile_shaders(
        ${TARGET}
        "${CMAKE_CURRENT_SOURCE_DIR}/engine/shaders"
        "${APP_BIN_DIRECTORY}/engine/shaders"
        # the shaders of elements of a user interface, and effects of cameras
        FOLDERS ui effects
)
