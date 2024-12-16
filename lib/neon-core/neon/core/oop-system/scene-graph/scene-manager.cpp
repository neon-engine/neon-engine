#include "scene-manager.hpp"

#include <ranges>
#include <stack>

#include "node/camera-node.hpp"
#include "node/player-node.hpp"
#include "node/render-node.hpp"

core::SceneManager::SceneManager(
  RenderPipeline *render_pipeline,
  InputContext *input_context,
  WindowContext *window_context,
  const std::shared_ptr<Logger> &logger)
  : _window_context(window_context),
    _input_context(input_context),
    _render_pipeline(render_pipeline),
    _player(input_context)
{
  _root = new Node("root", {});
  _logger = logger;
}

void core::SceneManager::Initialize() const
{
  _logger->Info("Initializing the scene");

  // TODO [issues/4] create a NodeFactory class and use that to initialize nodes
  // avoid using the heap to avoid std::bad_alloc exceptions as well as make use of locality

  const auto cube = new RenderNode(
    "cube",
    Transform{},
    RenderInfo{
      .model_path = "assets/models/cube.obj",
      .shader_path = "assets/shaders/color",
      .color = {1.0f, 0.5f, 0.31f}
    },
    _render_pipeline);

  const auto light_cube = new RenderNode(
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
  const auto player = new PlayerNode(
    "player",
    Transform{
      .position = {0.f, 0.f, 2.0f}
    },
    _input_context);

  const auto camera = new CameraNode("camera", Transform{}, _render_pipeline);

  _root->AddChild(cube);
  _root->AddChild(light_cube);
  _root->AddChild(player);
  player->AddChild(camera);

  PostOrderTraversal([](Node *node) { node->Initialize(); });
  _logger->Info("Initialized scene!");
  _input_context->CenterAndHideCursor();
}

void core::SceneManager::Update() const
{
  const auto delta_time = _window_context->GetDeltaTime();

  PreOrderTraversal([delta_time](Node *node)
  {
    node->Update(delta_time);
  });

  _render_pipeline->RenderFrame();
}

void core::SceneManager::CleanUp() const
{
  _logger->Info("Cleaning up the scene");
  PostOrderTraversal([this](Node *node)
  {
    node->CleanUp();
    _logger->Debug("Deleting \"{}\"", node->GetName());
    delete node;
  });
}

void core::SceneManager::PreOrderTraversal(const std::function<void(Node *)> &func) const
{
  std::stack<Node *> stack{};
  stack.push(_root);

  while (!stack.empty())
  {
    const auto current = stack.top();
    stack.pop();

    func(current);

    for (auto children = current->GetChildren();
         auto &it : std::ranges::reverse_view(children))
    {
      stack.push(it);
    }
  }
}

void core::SceneManager::PostOrderTraversal(const std::function<void(Node *)> &func) const
{
  std::stack<Node *> stack1, stack2;
  stack1.push(_root);

  while (!stack1.empty())
  {
    auto current = stack1.top();
    stack1.pop();
    stack2.push(current);

    for (Node *child : current->GetChildren())
    {
      stack1.push(child);
    }
  }

  while (!stack2.empty())
  {
    const auto current = stack2.top();
    stack2.pop();
    func(current);
  }
}
