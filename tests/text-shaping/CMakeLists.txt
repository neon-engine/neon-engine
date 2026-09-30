# Text as an application puts it together: real fonts, FreeType that draws
# their glyphs, HarfBuzz that shapes, and the core that splits a text into
# its parts, places its glyphs, and keeps them in an atlas.
neon_add_functional_test(text-shaping
        SOURCES
        text-shaping.cpp
        LIBRARIES neon-core neon-freetype neon-stb
)

# The fonts are compiled into the test, so that it finds them wherever it is
# started from, and on a machine it was cross-compiled for.
set(TEST_FONTS_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/test-fonts.hpp")
set(TEST_FONT_FILES
        "${CMAKE_SOURCE_DIR}/app/NeonRuntime/assets/fonts/inter/Inter-Regular.ttf"
        "${CMAKE_SOURCE_DIR}/app/NeonRuntime/assets/fonts/noto/NotoSansArabic-Regular.ttf"
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

target_include_directories(text-shaping.test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
