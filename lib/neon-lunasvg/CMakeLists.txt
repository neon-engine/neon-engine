set(TARGET neon-lunasvg)

# LunaSVG is built from its submodule as a static library, with PlutoVG,
# which it brings with it. Neither looks for a library on the machine. They
# are told not to look for fonts on the machine either, so that an image is
# drawn the same everywhere.
set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(PLUTOVG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(USE_SYSTEM_PLUTOVG OFF CACHE BOOL "" FORCE)
set(LUNASVG_DISABLE_LOAD_SYSTEM_FONTS ON CACHE BOOL "" FORCE)
set(PLUTOVG_DISABLE_FONT_FACE_CACHE_LOAD ON CACHE BOOL "" FORCE)
add_subdirectory(
        "${CMAKE_SOURCE_DIR}/external/lunasvg"
        "${CMAKE_CURRENT_BINARY_DIR}/lunasvg"
        EXCLUDE_FROM_ALL)

add_library(${TARGET} STATIC
        neon/image/luna-vector-image-rasterizer.cpp
        neon/image/luna-vector-image-rasterizer.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# LunaSVG is private. What includes the header of this library sees no
# declaration of LunaSVG and cannot call it.
target_link_libraries(${TARGET}
        PUBLIC neon-core
        PRIVATE lunasvg::lunasvg
)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/image/luna-vector-image-rasterizer.test.cpp)
endif ()
