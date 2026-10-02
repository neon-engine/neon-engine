set(TARGET neon-ryml)

add_library(${TARGET} STATIC
        neon/data/ryml-document-format.cpp
        neon/data/ryml-document-format.hpp
)
neon_warnings(${TARGET})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# rapidyaml is private. What includes ryml-document-format.hpp sees no
# declaration of rapidyaml and cannot call it.
target_link_libraries(${TARGET}
        PUBLIC neon-core
        PRIVATE ryml
)

if (NEON_BUILD_TESTS)
  neon_add_unit_test(${TARGET} neon/data/ryml-document-format.test.cpp)
endif ()
