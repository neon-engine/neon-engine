set(TARGET core)

file(GLOB source_files "${TARGET}/*.cpp" "${TARGET}/*.hpp" "private/*.cpp" "private/*.hpp")

add_library(${TARGET} STATIC ${source_files})

target_include_directories(${TARGET}
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private
)