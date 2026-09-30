set(TARGET neon-flecs)

add_library(${TARGET} STATIC
        neon/world-system/flecs-entity-store.cpp
        neon/world-system/flecs-entity-store.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Flecs is private. What includes flecs-entity-store.hpp sees no declaration
# of Flecs and cannot call it.
target_link_libraries(${TARGET}
        PUBLIC neon-core
        PRIVATE flecs::flecs_static
)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/world-system/flecs-entity-store.test.cpp)
endif ()
