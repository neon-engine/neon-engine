set(TARGET neon-platform)

# The small calls into the operating system that no other backend library
# covers, behind the interfaces of neon-core. SDL2 has no secure random
# numbers, so they are asked of the operating system here.
add_library(${TARGET} STATIC
        neon/random/os-entropy.cpp
        neon/random/os-entropy.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        PUBLIC neon-core
)

if (WIN32)
  # BCryptGenRandom, the secure generator of Windows, is in bcrypt.dll
  target_link_libraries(${TARGET} PRIVATE bcrypt)
endif ()

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/random/os-entropy.test.cpp)
endif ()
