set(TARGET neon-stb)

add_library(${TARGET} STATIC
        neon/text/stb-font-rasterizer.cpp
        neon/text/stb-font-rasterizer.hpp
        neon/image/stb-image-decoder.cpp
        neon/image/stb-image-decoder.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# stb is private. What includes stb-font-rasterizer.hpp sees no declaration
# of stb_truetype and cannot call it.
target_link_libraries(${TARGET}
        PUBLIC neon-core
        PRIVATE stb
)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/text/stb-font-rasterizer.test.cpp)

  # The test draws characters of a real font. The font is compiled into the
  # test, so that the test finds it wherever it is started from, and on a
  # machine it was cross-compiled for.
  set(TEST_FONT "${CMAKE_SOURCE_DIR}/app/NeonRuntime/assets/fonts/inter/Inter-Regular.ttf")
  set(TEST_FONT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/test-font.hpp")

  if (NOT EXISTS "${TEST_FONT_HEADER}" OR "${TEST_FONT}" IS_NEWER_THAN "${TEST_FONT_HEADER}")
    file(READ "${TEST_FONT}" TEST_FONT_BYTES HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," TEST_FONT_BYTES "${TEST_FONT_BYTES}")
    file(WRITE "${TEST_FONT_HEADER}"
            "// Written by CMake from ${TEST_FONT}\n"
            "inline constexpr unsigned char test_font[] = {${TEST_FONT_BYTES}};\n")
  endif ()

  target_include_directories(${NEON_UNIT_TEST_TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")

  neon_add_unit_test(${TARGET} neon/image/stb-image-decoder.test.cpp)

  # the same for images
  set(TEST_IMAGES_HEADER "${CMAKE_CURRENT_BINARY_DIR}/generated/test-images.hpp")
  set(TEST_IMAGE_FILES
          "${CMAKE_SOURCE_DIR}/app/NeonRuntime/assets/ui/heart.png"
          "${CMAKE_SOURCE_DIR}/app/NeonRuntime/assets/ui/test-card.jpg"
  )
  set(TEST_IMAGE_NAMES png_image jpeg_image)

  set(IS_STALE FALSE)
  foreach (TEST_IMAGE IN LISTS TEST_IMAGE_FILES)
    if (NOT EXISTS "${TEST_IMAGES_HEADER}" OR "${TEST_IMAGE}" IS_NEWER_THAN "${TEST_IMAGES_HEADER}")
      set(IS_STALE TRUE)
    endif ()
  endforeach ()

  if (IS_STALE)
    set(TEST_IMAGES_TEXT "// Written by CMake from the images of the runtime\n")
    foreach (TEST_IMAGE TEST_IMAGE_NAME IN ZIP_LISTS TEST_IMAGE_FILES TEST_IMAGE_NAMES)
      file(READ "${TEST_IMAGE}" TEST_IMAGE_BYTES HEX)
      string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," TEST_IMAGE_BYTES "${TEST_IMAGE_BYTES}")
      string(APPEND TEST_IMAGES_TEXT
              "inline constexpr unsigned char ${TEST_IMAGE_NAME}[] = {${TEST_IMAGE_BYTES}};\n")
    endforeach ()
    file(WRITE "${TEST_IMAGES_HEADER}" "${TEST_IMAGES_TEXT}")
  endif ()

  target_include_directories(${NEON_UNIT_TEST_TARGET} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
endif ()
