#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "graphics/shader.hpp"
#include "graphics/triangle.hpp"
#include "graphics/plane.hpp"

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

  Shader base_shader(
      "shaders/base-shader.vert",
      "shaders/base-shader.frag");

  Shader textured_shader(
      "shaders/textured-shader.vert",
      "shaders/textured-shader.frag");

  int tex_width, tex_height, nr_channels;
  stbi_set_flip_vertically_on_load(true);
  unsigned char *data = stbi_load(
      "assets/textures/concrete.png",
      &tex_width,
      &tex_height,
      &nr_channels,
      STBI_rgb);

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  if (data) {
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 tex_width,
                 tex_height,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);
  } else {
    std::cout << "Failed to load texture" << std::endl;
  }

  // free the memory after we are done creating texture
  stbi_image_free(data);

  glBindTexture(GL_TEXTURE_2D, 0);

  Plane plane;

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

    textured_shader.Use();
    glBindTexture(GL_TEXTURE_2D, texture);
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