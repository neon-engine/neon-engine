#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "graphics/shader.hpp"
#include "graphics/plane.hpp"
#include "graphics/texture.hpp"

void ProcessInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // prevent window from resizing
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    constexpr int width = 800;
    constexpr int height = 600;

    // Create the window
    GLFWwindow* window = glfwCreateWindow(
        width,
        height,
        "LearnOpenGL",
        nullptr,
        nullptr);

    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    if (gl3wInit())
    {
        std::cerr << "Failed to initialize gl3w" << std::endl;
        glfwTerminate();
        return -1;
    }

    int num_vertex_attributes_supported;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_vertex_attributes_supported);
    std::cout << "Maximum number of vertex attributes supported: " << num_vertex_attributes_supported << std::endl;

    glViewport(0, 0, width, height);

    const Shader base_shader(
        "assets/shaders/base-shader.vert",
        "assets/shaders/base-shader.frag");

    const Shader textured_shader(
        "assets/shaders/textured-shader.vert",
        "assets/shaders/textured-shader.frag");

    const Texture texture("assets/textures/concrete.png");



    while (!glfwWindowShouldClose(window))
    {
        Plane plane;

        // input
        ProcessInput(window);

        // If we ever cared to render in wireframe the following code would enable us to toggle it on and off
        // if (wireframe) {
        //   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        // } else {
        //   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // }

        // rendering commands
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        textured_shader.Use();
        texture.Use();
        plane.Draw();
        //    triangle.Draw();

        // check and call events and swap the buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

#pragma clang diagnostic pop
