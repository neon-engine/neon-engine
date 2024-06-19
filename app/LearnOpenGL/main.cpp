#include <iostream>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "graphics/cube.hpp"
#include "graphics/plane.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/triangle.hpp"

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

    glEnable(GL_DEPTH_TEST);

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

    // ReSharper disable once CppTooWideScope
    const Cube cube;

    constexpr glm::vec3 cube_positions[] = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f, 3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f, 2.0f, -2.5f),
        glm::vec3(1.5f, 0.2f, -1.5f),
        glm::vec3(-1.3f, 1.0f, -1.5f)
    };

    while (!glfwWindowShouldClose(window))
    {
        process_input(window);

        if (wireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view(1.0f);
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

        glm::mat4 projection(1.0f);
        projection = glm::perspective(
            glm::radians(45.0f),
            static_cast<float>(width)/static_cast<float>(height),
            0.1f,
            100.0f);

        const auto shader_id = mix_textured_shader.GetId();
        const auto model_loc = glGetUniformLocation(shader_id, "model");
        const auto view_loc = glGetUniformLocation(shader_id, "view");
        const auto projection_loc = glGetUniformLocation(shader_id, "projection");

        texture1.Use(0);
        texture2.Use(1);
        mix_textured_shader.Use();

        for (int i = 0; i < 10; i++)
        {
            glm::mat4 model(1.0f);
            model = translate(model, cube_positions[i]);
            const float angle = 20.0f * static_cast<float>(i);
            model = rotate(model,
                glm::radians(angle),
                glm::vec3(0.5f, 1.0f, 0.0f));
            glUniformMatrix4fv(model_loc, 1, GL_FALSE, value_ptr(model));
            glUniformMatrix4fv(view_loc, 1, GL_FALSE, value_ptr(view));
            glUniformMatrix4fv(projection_loc, 1, GL_FALSE, value_ptr(projection));
            cube.Draw();
        }

        // check and call events and swap the buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
