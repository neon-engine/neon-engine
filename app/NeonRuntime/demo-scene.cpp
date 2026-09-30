#include "demo-scene.hpp"

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>

void DemoScene::Populate(neon::EntityStore &store)
{
  const auto bear = store.CreateEntity("bear");
  store.Set(bear, neon::Transform{});
  store.Set(bear, neon::Renderable{
    .render_info = {
      .model_path = "assets://models/bear.obj",
      .shader_path = "assets://shaders/basic-lit",
      .texture_paths = {
        "assets://textures/concrete.png"
      },
      .material_info = {
        .shininess = 32.f,
        .color = {1.0f, 0.5f, 0.31f}
      },
    }
  });

  const auto sphere = store.CreateEntity("sphere");
  store.Set(sphere, neon::Transform{
    .position = {1.2f, 1.0f, -2.0f},
    .scale = glm::vec3{0.2f}
  });
  store.Set(sphere, neon::Renderable{
    .render_info = {
      .model_path = "assets://models/sphere.obj",
      .shader_path = "assets://shaders/basic-lit",
      .texture_paths = {
        "assets://textures/fire.png"
      },
      .material_info = {
        .shininess = 100.f,
        .color = {1.0f, 1.0f, 1.0f},
        .use_textures = false
      },
    }
  });

  const auto floor = store.CreateEntity("floor");
  store.Set(floor, neon::Transform{
    .position = glm::vec3{0.f, -.55f, 0.f},
    .scale = glm::vec3{100.f, .1f, 100.f}
  });
  store.Set(floor, neon::Renderable{
    .render_info = {
      .model_path = "assets://models/cube.obj",
      .shader_path = "assets://shaders/basic-lit",
      .texture_paths = {
        "assets://textures/concrete.png"
      },
      .scale_textures = true,
      .material_info = {
        .shininess = 32.f,
        .color = {0.5f, 0.5f, 0.5f}
      },
    }
  });

  const auto direction_light = store.CreateEntity("direction");
  store.Set(direction_light, neon::Transform{
    .position = {1.2f, 1.0f, -2.0f}
  });
  store.Set(direction_light, neon::Light{
    .source = {
      .id = "direction",
      .light_type = neon::LightType::Direction,
      .direction = glm::vec3(-0.2f, -1.0f, -0.3f),
      .ambient = glm::vec3(0.5f, 0.5f, 0.5f),
      .diffuse = glm::vec3(0.4f, 0.4f, 0.4f),
      .specular = glm::vec3(0.5f, 0.5f, 0.5f)
    }
  });

  const auto player = store.CreateEntity("player");
  store.Set(player, neon::Transform{
    .position = {0.f, 0.f, 2.0f}
  });
  store.Set(player, neon::Spectator{});

  // the camera sits on the player, and so moves and turns with it
  const auto camera = store.CreateEntity("camera", player);
  store.Set(camera, neon::Transform{});
  store.Set(camera, neon::Camera{});
}
