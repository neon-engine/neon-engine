add_executable(TestApp main.cpp)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(gl3w REQUIRED)

target_link_libraries(TestApp glm glfw3 gl3w)
