#include "scene-loader.hpp"

#include "render-node.hpp"

core::SceneLoader::SceneLoader(
  RenderPipeline *render_pipeline,
  InputContext *input_context,
  WindowContext *window_context)
  : _window_context(window_context),
    _input_context(input_context),
    _render_pipeline(render_pipeline),
    _player(input_context),
    _scene_graph("root", {}) {}

void core::SceneLoader::Initialize()
{
  std::cout << "Initializing the scene" << std::endl;

  const auto cube = std::make_shared<RenderNode>(
    "cube",
    Transform{},
    RenderInfo{
      .model_path = "assets/models/cube.obj",
      .shader_path = "assets/shaders/color",
      .color = {1.0f, 0.5f, 0.31f}
    },
    _render_pipeline);

  const auto light_cube = std::make_shared<RenderNode>(
    "light cube",
    Transform{
      .position = {1.2f, 1.0f, -2.0f},
      .scale = glm::vec3{0.2f}
    },
    RenderInfo{
      .model_path = "assets/models/cube.obj",
      .shader_path = "assets/shaders/color",
      .color = {1.0f, 1.0f, 1.0f}
    },
    _render_pipeline);
  _scene_graph.AddChild(cube);
  _scene_graph.AddChild(light_cube);
  _scene_graph.Initialize();
  std::cout << "Initialized scene!" << std::endl;
  _input_context->CenterAndHideCursor();
}

void core::SceneLoader::Update()
{
  const auto delta_time = _window_context->GetDeltaTime();
  _player.Update(delta_time, glm::mat4{1.0f});
}

void core::SceneLoader::RenderFrame() const
{
}

void core::SceneLoader::CleanUp()
{
  std::cout << "Cleaning up the scene" << std::endl;
  _scene_graph.CleanUp();
}
