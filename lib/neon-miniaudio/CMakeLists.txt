set(TARGET neon-miniaudio)

add_library(${TARGET} STATIC
        neon/audio/ma-audio-system.cpp
        neon/audio/ma-audio-system.hpp
        neon/audio/miniaudio-implementation.c
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# miniaudio is private. What includes ma-audio-system.hpp sees no declaration
# of miniaudio and cannot call it. It is a system header, so that the warnings
# of the engine stop at it: its implementation is compiled into the file that
# includes it.
target_include_directories(${TARGET} SYSTEM
        PRIVATE ${CMAKE_SOURCE_DIR}/external/miniaudio
)

# Decoders that need a library of their own stay off. WAV, FLAC, and MP3 are
# part of miniaudio, and Ogg Vorbis comes from the stb_vorbis it ships.
target_compile_definitions(${TARGET} PRIVATE
        MA_NO_ENCODING
)

target_link_libraries(${TARGET}
        PUBLIC neon-core
)

if (UNIX AND NOT APPLE)
  # miniaudio loads the sound libraries of the system when it starts
  find_package(Threads REQUIRED)
  target_link_libraries(${TARGET} PRIVATE Threads::Threads ${CMAKE_DL_LIBS} m)
endif ()

if (NEON_BUILD_TESTS)
  # The audio is mixed without a sound card, so the tests run anywhere and
  # measure what would be heard.
  neon_add_unit_test(${TARGET} neon/audio/ma-audio-system.test.cpp)
endif ()
