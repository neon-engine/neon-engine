#include <iostream>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "graphics/plane.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"

bool wireframe = false;
bool keydown = false;

static void process_input(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

static void process_input_callback(GLFWwindow* window,
    const int key,
    const int scancode,
    const int action,
    const int mods)
{
    if (key == GLFW_KEY_RIGHT_ALT && action == GLFW_PRESS)
    {
        wireframe = !wireframe;
        std::cout << "toggling wireframe: " << wireframe << std::endl;
    }
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
    GLFWwindow *window = glfwCreateWindow(
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

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, process_input_callback);

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

    const Shader mix_textured_shader(
        "assets/shaders/mix-textured-shader.vert",
        "assets/shaders/mix-textured-shader.frag");

    const Texture texture1("assets/textures/container.jpg");
    const Texture texture2("assets/textures/awesomeface.png", GL_RGBA);

    mix_textured_shader.Use();
    mix_textured_shader.SetInt("texture1", 0);
    mix_textured_shader.SetInt("texture2", 1);


    while (!glfwWindowShouldClose(window))
    {
        Plane plane;

        process_input(window);

        if (wireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 trans(1.0f);
        trans = translate(trans, glm::vec3(0.5, -0.5f, 0.0f));
        trans = rotate(trans, static_cast<float>(glfwGetTime()), glm::vec3(0.0, 0.0, 1.0));

        const auto transform_loc = glGetUniformLocation(mix_textured_shader.GetId(), "transform");
        glUniformMatrix4fv(transform_loc, 1, GL_FALSE, value_ptr(trans));

        texture1.Use(0);
        texture2.Use(1);
        mix_textured_shader.Use();
        plane.Draw();

        // check and call events and swap the buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
