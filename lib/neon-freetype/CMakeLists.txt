set(TARGET neon-freetype)

# FreeType and HarfBuzz are built from their submodules, as static
# libraries, and with everything switched off that they would otherwise
# look for on the machine. The build is then the same on macOS, on Linux,
# and when cross-compiling for Windows.

# FreeType draws glyphs. Compressed fonts, bitmaps in PNG, and WOFF2 are
# left out, since each needs a library of its own. HarfBuzz is left out
# here because FreeType would use it for hinting alone, and because the two
# would otherwise depend on each other.
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_ENABLE_ERROR_STRINGS OFF CACHE BOOL "" FORCE)

# for this folder alone: other libraries read a variable of the same name
set(SKIP_INSTALL_ALL ON)
add_subdirectory(
        "${CMAKE_SOURCE_DIR}/external/freetype"
        "${CMAKE_CURRENT_BINARY_DIR}/freetype"
        EXCLUDE_FROM_ALL)

# HarfBuzz shapes text. It is compiled from the one file it offers for
# that, in place of its own build, which looks for Python, threads, and a
# handful of libraries. It reads fonts with its own OpenType functions, and
# knows nothing of FreeType, of ICU, of glib, or of what a platform brings
# for shaping.
add_library(neon-harfbuzz STATIC
        "${CMAKE_SOURCE_DIR}/external/harfbuzz/src/harfbuzz.cc"
)

target_include_directories(neon-harfbuzz SYSTEM
        PUBLIC "${CMAKE_SOURCE_DIR}/external/harfbuzz/src"
)

target_compile_definitions(neon-harfbuzz
        # the locks of the standard library, which every platform has
        PRIVATE HB_MUTEX_IMPL_STD_MUTEX
        PRIVATE HB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR
)

# HarfBuzz is written without exceptions and without information about
# types at run time
target_compile_options(neon-harfbuzz PRIVATE -fno-exceptions -fno-rtti -w)
set_target_properties(neon-harfbuzz PROPERTIES CXX_STANDARD 17)

add_library(${TARGET} STATIC
        neon/text/ft-font-rasterizer.cpp
        neon/text/ft-font-rasterizer.hpp
        neon/text/hb-text-shaper.cpp
        neon/text/hb-text-shaper.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Both are private. What includes the headers of this library sees no
# declaration of FreeType or HarfBuzz and cannot call them.
target_link_libraries(${TARGET}
        PUBLIC neon-core
        PRIVATE freetype
        PRIVATE neon-harfbuzz
)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/text/ft-font-rasterizer.test.cpp)
  set(FIRST_TEST ${NEON_UNIT_TEST_TARGET})
  neon_add_unit_test(${TARGET} neon/text/hb-text-shaper.test.cpp)

  # The tests draw and shape with real fonts. The fonts are compiled into
  # the tests, so that a test finds them wherever it is started from, and
  # on a machine it was cross-compiled for.
  set(TEST_FONTS_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/test-fonts.hpp")
  set(TEST_FONT_FILES
          "${CMAKE_SOURCE_DIR}/app/NeonRuntime/engine/fonts/inter/Inter-Regular.ttf"
          "${CMAKE_SOURCE_DIR}/app/NeonRuntime/engine/fonts/noto/NotoSansArabic-Regular.ttf"
  )
  set(TEST_FONT_NAMES inter_regular noto_sans_arabic)

  set(IS_STALE FALSE)
  foreach (TEST_FONT IN LISTS TEST_FONT_FILES)
    if (NOT EXISTS "${TEST_FONTS_HEADER}" OR "${TEST_FONT}" IS_NEWER_THAN "${TEST_FONTS_HEADER}")
      set(IS_STALE TRUE)
    endif ()
  endforeach ()

  if (IS_STALE)
    set(TEST_FONTS_TEXT "// Written by CMake from the fonts of the runtime\n")
    foreach (TEST_FONT TEST_FONT_NAME IN ZIP_LISTS TEST_FONT_FILES TEST_FONT_NAMES)
      file(READ "${TEST_FONT}" TEST_FONT_BYTES HEX)
      string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," TEST_FONT_BYTES "${TEST_FONT_BYTES}")
      string(APPEND TEST_FONTS_TEXT
              "inline constexpr unsigned char ${TEST_FONT_NAME}[] = {${TEST_FONT_BYTES}};\n")
    endforeach ()
    file(WRITE "${TEST_FONTS_HEADER}" "${TEST_FONTS_TEXT}")
  endif ()

  target_include_directories(${FIRST_TEST} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
  target_include_directories(${NEON_UNIT_TEST_TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
endif ()
