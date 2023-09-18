#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <iostream>

const char *vertex_shader_source = "#version 330 core\n"
                                   "layout (location = 0) in vec3 aPos;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                   "}\0";

const char *fragment_shader_source = "#version 330 core\n"
                                     "out vec4 FragColor;\n"
                                     "void main()\n"
                                     "{\n"
                                     "  FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
                                     "}\0";

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

  glViewport(0, 0, width, height);

  // triangle
  float vertices[] = {
      -0.5f, -0.5f, 0.0f,
      0.5f, -0.5f, 0.0f,
      0.0f, 0.5f, 0.0f
  };

  unsigned int vertex_shader, fragment_shader;
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

  glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
  glCompileShader(vertex_shader);
  glCompileShader(fragment_shader);

  int success;
  int buffer_size = 512;
  char info_log[buffer_size];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(vertex_shader, buffer_size, nullptr, info_log);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << info_log << std::endl;
  }

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    glGetShaderInfoLog(fragment_shader, buffer_size, nullptr, info_log);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << info_log << std::endl;
  }

  unsigned int shader_program;
  shader_program = glCreateProgram();

  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program); // link shaders to the shader program

  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, nullptr, info_log);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << std::endl;
  }

  // delete shaders, we no longer need them after linking
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  unsigned int vbo, vao;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
  glEnableVertexAttribArray(0);

  while (!glfwWindowShouldClose(window)) {
    // input
    ProcessInput(window);

    // rendering commands
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // check and call events and swap the buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

