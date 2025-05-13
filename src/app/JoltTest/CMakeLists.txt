set(TARGET JoltTest)

add_executable(${TARGET} main.cpp)

find_package(Threads REQUIRED)

target_link_libraries(${TARGET} PRIVATE Threads::Threads Jolt)
