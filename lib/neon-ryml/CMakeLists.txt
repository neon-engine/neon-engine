set(TARGET neon-ryml)

add_library(${TARGET} STATIC
        neon/data/ryml-document-format.cpp
        neon/data/ryml-document-format.hpp
)

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# rapidyaml is private. What includes ryml-document-format.hpp sees no
# declaration of rapidyaml and cannot call it.
target_link_libraries(${TARGET}
        PUBLIC neon-core
        PRIVATE ryml
)
