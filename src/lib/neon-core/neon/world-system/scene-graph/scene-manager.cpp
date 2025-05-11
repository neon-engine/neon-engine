#include "scene-manager.hpp"

#include <ranges>
#include <stack>

#include "node/camera-node.hpp"
#include "node/light-node.hpp"
#include "node/render-node.hpp"
#include "node/spectator-node.hpp"

namespace neon
{
  SceneManager::SceneManager(
    RenderPipeline *render_pipeline,
    InputContext *input_context,
    WindowContext *window_context,
    const std::shared_ptr<Logger> &logger)
    : WorldSystem(render_pipeline, input_context, window_context, logger),
      _window_context(window_context),
      _input_context(input_context),
      _render_pipeline(render_pipeline)
  {
    _root = new Node("root", {}, logger);
    _logger = logger;
  }

  void SceneManager::Initialize()
  {
    _logger->Info("Initializing the scene");

    // TODO [issues/4] create a NodeFactory class and use that to initialize nodes
    // avoid using the heap to avoid std::bad_alloc exceptions as well as make use of locality

    const auto bear = new RenderNode(
      "bear",
      Transform{},
      RenderInfo{
        .model_path = "assets/models/bear.obj",
        .shader_path = "assets/shaders/basic-lit",
        .texture_paths = {
          "assets/textures/concrete.png"
        },
        .material_info = {
          .shininess = 32.f,
          .color = {1.0f, 0.5f, 0.31f}
        },
      },
      _render_pipeline,
      _logger);

    const auto sphere = new RenderNode(
      "sphere",
      Transform{
        .position = {1.2f, 1.0f, -2.0f},
        .scale = glm::vec3{0.2f}
      },
      RenderInfo{
        .model_path = "assets/models/sphere.obj",
        .shader_path = "assets/shaders/basic-lit",
        .texture_paths = {
          "assets/textures/fire.png"
        },
        .material_info = {
          .shininess = 100.f,
          .color = {1.0f, 1.0f, 1.0f},
          .use_textures = false
        },
      },
      _render_pipeline,
      _logger);

    const auto floor = new RenderNode(
      "floor",
      Transform{
        .position = glm::vec3{0.f, -.55f, 0.f},
        .scale = glm::vec3{100.f, .1f, 100.f}
      },
      RenderInfo{
        .model_path = "assets/models/cube.obj",
        .shader_path = "assets/shaders/basic-lit",
        .texture_paths = {
          "assets/textures/concrete.png"
        },
        .scale_textures = true,
        .material_info = {
          .shininess = 32.f,
          .color = {0.5f, 0.5f, 0.5f}
        },
      },
      _render_pipeline,
      _logger);

    const auto direction_light = new LightNode(
      "direction",
      Transform{
        .position = {1.2f, 1.0f, -2.0f}
      },
      _render_pipeline,
      LightSource{
        .id = "direction",
        .light_type = LightType::Direction,
        .direction = glm::vec3(-0.2f, -1.0f, -0.3f),
        .ambient = glm::vec3(0.5f, 0.5f, 0.5f),
        .diffuse = glm::vec3(0.4f, 0.4f, 0.4f),
        .specular = glm::vec3(0.5f, 0.5f, 0.5f)
      },
      _logger
    );

    const auto player = new SpectatorNode(
      "player",
      Transform{
        .position = {0.f, 0.f, 2.0f}
      },
      _input_context,
      _logger);

    const auto camera = new CameraNode("camera", Transform{}, _render_pipeline, _logger);

    _root->AddChild(bear);
    _root->AddChild(floor);
    _root->AddChild(direction_light);
    _root->AddChild(player);
    _root->AddChild(sphere);
    player->AddChild(camera);

    PostOrderTraversal([](Node *node) { node->Initialize(); });
    _logger->Info("Initialized scene!");
    _input_context->CenterAndHideCursor();
  }

  void SceneManager::Update()
  {
    const auto delta_time = _window_context->GetDeltaTime();

    PreOrderTraversal([delta_time](Node *node)
    {
      node->Update(delta_time);
    });

    _render_pipeline->RenderFrame();
  }

  void SceneManager::CleanUp()
  {
    _logger->Info("Cleaning up the scene");
    PostOrderTraversal([this](Node *node)
    {
      node->CleanUp();
      _logger->Debug("Deleting \"{}\"", node->GetName());
      delete node;
    });
  }

  void SceneManager::PreOrderTraversal(const std::function<void(Node *)> &func) const
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

  void SceneManager::PostOrderTraversal(const std::function<void(Node *)> &func) const
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
}
