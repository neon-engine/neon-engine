# Jolt Physics, the library behind neon-jolt. Only the library itself is
# built. Its samples, unit tests, and viewer are declared by Jolt when it is
# the top of a build only, which it is not here.
#
# Every option is set before Jolt is added, since Jolt reads them while it is
# configured. What Jolt declares PUBLIC, such as the instruction sets, reaches
# neon-jolt through the target, so that both are compiled the same way.

# Jolt turns its warnings into errors, which breaks the build whenever a
# compiler learns a new warning. Its warnings are not ours to fix.
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)

# Jolt replaces the flags of a build type with its own otherwise. It is
# built with the flags of the project, as every other library is.
set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)

# The same input gives the same result on every platform and compiler, so
# that a replay or a test that was recorded on one machine holds on another.
# It costs a few percent of speed, and turns fused multiply-add off.
set(CROSS_PLATFORM_DETERMINISTIC ON CACHE BOOL "" FORCE)

# neon-jolt derives from classes of Jolt and is compiled with run-time type
# information and exceptions, as the engine is. Jolt has to be compiled the
# same way, or the type information of its classes is missing when linking.
set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE)
set(CPP_EXCEPTIONS_ENABLED ON CACHE BOOL "" FORCE)

# On x86 Jolt asks for AVX2 by default, and what is built with it does not
# start on a processor without. SSE 4.2 is what every 64-bit processor of
# the last fifteen years has. ARM needs no flags and ignores these.
set(USE_SSE4_1 ON CACHE BOOL "" FORCE)
set(USE_SSE4_2 ON CACHE BOOL "" FORCE)
set(USE_AVX OFF CACHE BOOL "" FORCE)
set(USE_AVX2 OFF CACHE BOOL "" FORCE)
set(USE_AVX512 OFF CACHE BOOL "" FORCE)
set(USE_LZCNT OFF CACHE BOOL "" FORCE)
set(USE_TZCNT OFF CACHE BOOL "" FORCE)
set(USE_F16C OFF CACHE BOOL "" FORCE)
set(USE_FMADD OFF CACHE BOOL "" FORCE)

# Link time optimization of a static library ties it to the linker of the
# compiler that built it, which the cross-compiled build cannot rely on.
set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)

# Parts of Jolt the engine does not use.
set(DEBUG_RENDERER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(PROFILER_IN_DEBUG_AND_RELEASE OFF CACHE BOOL "" FORCE)
set(ENABLE_OBJECT_STREAM OFF CACHE BOOL "" FORCE)
set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../external/jolt-physics/Build" "${PROJECT_BINARY_DIR}/external/jolt-physics/Build")
