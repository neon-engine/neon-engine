set(TARGET NeonFPS)

add_executable(${TARGET} main.cpp
        neon-fps-application.cpp
        neon-fps-application.hpp)

target_link_libraries(${TARGET} PRIVATE core)
