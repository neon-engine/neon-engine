set(TARGET neon-runtime)

# The start-up NeonRuntime and NeonEditor share: RuntimeComposition walks it
# as named steps, and CompositionHooks is where an application differs. It
# is the one library that names the backends, since it composes them, and
# it is a neon-* target itself, so an application still names only neon-*
# targets. See docs/development.md, "How an application starts".
add_library(${TARGET} STATIC
        neon/runtime/application-description.hpp
        neon/runtime/composition-hooks.hpp
        neon/runtime/runtime-systems.hpp
        neon/runtime/runtime-composition.cpp
        neon/runtime/runtime-composition.hpp

        # the steps that are more than a few lines, each in a file of its
        # own, on the interfaces of neon-core so that they are tested with
        # the fakes
        neon/runtime/input-map-loading.cpp
        neon/runtime/input-map-loading.hpp
        neon/runtime/input-script-loading.cpp
        neon/runtime/input-script-loading.hpp
        neon/runtime/project-reading.cpp
        neon/runtime/project-reading.hpp
        neon/runtime/settings-layers.cpp
        neon/runtime/settings-layers.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Public, since the hooks hand an application the backends behind the
# window, the input, and the renderer, for what it draws with them. What the
# platform starts the application by, neon-sdl2-main, is not here: it brings
# SDL's own entry point, which has to be linked into the executable, and a
# test of this library has the entry point of GoogleTest instead.
target_link_libraries(${TARGET}
        PUBLIC
        neon-core
        neon-vulkan
        neon-sdl2
        neon-flecs
        neon-ryml
        neon-miniaudio
        neon-jolt
        neon-lua
        neon-stb
        neon-freetype
        neon-lunasvg
        neon-platform
)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/runtime/project-reading.test.cpp)
  neon_add_unit_test(${TARGET} neon/runtime/settings-layers.test.cpp)
  neon_add_unit_test(${TARGET} neon/runtime/input-map-loading.test.cpp)
  neon_add_unit_test(${TARGET} neon/runtime/input-script-loading.test.cpp)
endif ()
