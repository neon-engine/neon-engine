set(TARGET neon-jolt)

add_library(${TARGET} STATIC
        neon/physics/jolt-physics-system.cpp
        neon/physics/jolt-physics-system.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Jolt is private. What includes jolt-physics-system.hpp sees no declaration
# of Jolt and cannot call it.
#
# Jolt declares how it was compiled on its target: the instruction sets, and
# whether the same input has to give the same result on every platform. What
# includes its headers has to be compiled the same way, and is, since a
# private link hands these on to this library and no further.
target_link_libraries(${TARGET}
        PUBLIC neon-core
        PRIVATE Jolt
)

# The same result on every platform needs every sum and product to be
# rounded by itself. Jolt is compiled that way, and so is what is compiled
# here from its headers.
if (NOT MSVC)
  target_compile_options(${TARGET} PRIVATE -ffp-contract=off)
endif ()

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/physics/jolt-physics-system.test.cpp)
endif ()
