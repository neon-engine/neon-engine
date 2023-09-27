#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "graphics/shaders/shader.hpp"
#include "graphics/geometry/triangle.hpp"

void ProcessInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

int main() {

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return -1;
  }

  // Configure GLFW
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // prevent window from resizing
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  int width = 800;
  int height = 600;

  // Create the window
  GLFWwindow *window = glfwCreateWindow(
      width,
      height,
      "LearnOpenGL",
      nullptr,
      nullptr);

  if (window == nullptr) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  // Make the window's context current
  glfwMakeContextCurrent(window);

  if (gl3wInit()) {
    std::cerr << "Failed to initialize gl3w" << std::endl;
    glfwTerminate();
    return -1;
  }

  int num_vertex_attributes_supported;
  glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_vertex_attributes_supported);
  std::cout << "Maximum number of vertex attributes supported: " << num_vertex_attributes_supported << std::endl;

  glViewport(0, 0, width, height);

  Triangle triangle;

  // triangle
  float vertices[] = {
      0.5f, 0.5f, 0.0f,  // top right
      0.5f, -0.5f, 0.0f,  // bottom right
      -0.5f, -0.5f, 0.0f,  // bottom left
      -0.5f, 0.5f, 0.0f   // top left
  };
  unsigned int indices[] = {  // note that we start from 0!
      0, 1, 3,   // first triangle
      1, 2, 3    // second triangle
  };

  Shader base_shader("shaders/base-shader.vert", "shaders/base-shader.frag");

  unsigned int vao, vbo, ebo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) nullptr);
  glEnableVertexAttribArray(0);

  while (!glfwWindowShouldClose(window)) {
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

    base_shader.Use();
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    // check and call events and swap the buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
