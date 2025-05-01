set(TARGET neon-vulkan)

add_library(${TARGET} STATIC
        neon/render/vulkan-render-system.cpp
        neon/render/vulkan-render-system.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET}
        glm::glm
        stb
        neon-core
)
