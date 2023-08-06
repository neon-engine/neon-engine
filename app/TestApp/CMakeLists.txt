find_package(ZLIB REQUIRED)

add_executable(TestApp main.cpp)
target_link_libraries(TestApp PRIVATE ZLIB::ZLIB)