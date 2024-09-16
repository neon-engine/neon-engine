#include "scene.hpp"

#include <ranges>
#include <stack>

#include "node/render-node.hpp"

core::Scene::Scene(
  RenderPipeline *render_pipeline,
  InputContext *input_context,
  WindowContext *window_context)
  : _window_context(window_context),
    _input_context(input_context),
    _render_pipeline(render_pipeline),
    _player(input_context)
{
  _root = new Node("root", {});
}

void core::Scene::Initialize()
{
  std::cout << "Initializing the scene" << std::endl;

  // todo create a NodeFactory class and use that to initialize nodes

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
  _root->AddChild(cube);
  _root->AddChild(light_cube);
  _root->Initialize();
  std::cout << "Initialized scene!" << std::endl;
  _input_context->CenterAndHideCursor();
}

void core::Scene::Update() const
{
  const auto delta_time = _window_context->GetDeltaTime();

  PreOrderTraversal([delta_time](Node *node)
  {
    node->Update(delta_time);
  });
}

void core::Scene::CleanUp() const
{
  std::cout << "Cleaning up the scene" << std::endl;
  PostOrderTraversal([](Node *node)
  {
    node->CleanUp();
    std::cout << "Deleting " << node->GetName() << std::endl;
    delete node;
  });
}

void core::Scene::PreOrderTraversal(const std::function<void(Node *)> &func) const
{
  std::cout << "Processing scene graph in pre-order traversal" << std::endl;
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

  std::cout << "Done with pre-order traversal" << std::endl;
}

void core::Scene::PostOrderTraversal(const std::function<void(Node *)> &func) const
{
  std::cout << "Processing scene graph in post-order traversal" << std::endl;
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

  std::cout << "Done with post-order traversal" << std::endl;
}
