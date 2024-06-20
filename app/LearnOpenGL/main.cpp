#include <algorithm>
#include <iostream>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "graphics/graphics.hpp"

bool wireframe = false;
bool keydown = false;
auto camera_move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
constexpr float kSpeed = 2.5f;
constexpr float kMouseSensitivity = 0.1f;
int width = 800;
int height = 600;
float fov = 45.0f;
auto camera_pos = glm::vec3(0.0f, 0.0f, 3.0f);
auto camera_front = glm::vec3(0.0f, 0.0f, -1.0f);
auto camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
float last_x = static_cast<float>(width) / 2;
float last_y = static_cast<float>(height) / 2;
float yaw = -90.0f;
float pitch = 0.0f;
float x_offset = 0.0f;
float y_offset = 0.0f;
bool first_mouse = false;
auto look_direction = glm::vec3(0.0f, 0.0f, -1.0f);

static void process_input(GLFWwindow *window)
{
  camera_move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
  {
    glfwSetWindowShouldClose(window, true);
  }

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
  {
    camera_move_direction = look_direction * kSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
  {
    camera_move_direction = look_direction * -kSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
  {
    camera_move_direction = normalize(cross(look_direction, camera_up)) * -kSpeed;
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
  {
    camera_move_direction = normalize(cross(look_direction, camera_up)) * kSpeed;
  }
}

static void process_input_callback(
  GLFWwindow *window,
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

static void mouse_callback(GLFWwindow *window, const double x_pos, const double y_pos)
{
  if (first_mouse)
  {
    first_mouse = false;
    last_x = static_cast<float>(x_pos);
    last_y = static_cast<float>(y_pos);
  }

  x_offset = static_cast<float>(x_pos) - last_x;
  y_offset = last_y - static_cast<float>(y_pos);
  last_x = static_cast<float>(x_pos);
  last_y = static_cast<float>(y_pos);

  x_offset *= kMouseSensitivity;
  y_offset *= kMouseSensitivity;

  yaw += x_offset;
  pitch += y_offset;

  pitch = std::clamp(pitch, -89.0f, 89.0f);

  glm::vec3 direction;
  direction.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
  direction.y = glm::sin(glm::radians(pitch));
  direction.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
  look_direction = normalize(direction);
}

void scroll_callback(GLFWwindow *window, const double scroll_x_offset, const double scroll_y_offset)
{
  fov -= static_cast<float>(scroll_y_offset);
  fov = std::clamp(fov, 1.0f, 90.0f);
}

int main()
{
  glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

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

  // GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  // const GLFWvidmode* mode = glfwGetVideoMode(monitor);
  // width = mode->width;
  // height = mode->height;


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
  glfwSetCursorPosCallback(window, mouse_callback);
  glfwSetScrollCallback(window, scroll_callback);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
  Camera camera;

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

  float last_frame = 0.0f;

  while (!glfwWindowShouldClose(window))
  {
    const auto current_frame = static_cast<float>(glfwGetTime());
    const float delta_time = current_frame - last_frame;
    last_frame = current_frame;

    process_input(window);
    camera.Update(delta_time, camera_move_direction, look_direction);

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
    view = camera.GetView();

    glm::mat4 projection(1.0f);
    projection = glm::perspective(
      glm::radians(fov),
      static_cast<float>(width) / static_cast<float>(height),
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
