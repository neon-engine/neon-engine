#include "entity-world.hpp"

#include <neon/common/transform.hpp>

#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/renderable.hpp"
#include "components/spectator.hpp"
#include "systems/render-submission.hpp"
#include "systems/spectator-movement.hpp"
#include "systems/transform-propagation.hpp"

namespace neon
{
  EntityWorld::EntityWorld(
    EntityStore *store,
    Scene *scene,
    RenderPipeline *render_pipeline,
    InputContext *input_context,
    WindowContext *window_context,
    const std::shared_ptr<Logger> &logger)
    : WorldSystem(render_pipeline, input_context, window_context, logger)
  {
    _store = store;
    _scene = scene;

    _before.push_back(std::make_unique<SpectatorMovement>(input_context));
    _after.push_back(std::make_unique<TransformPropagation>());
    _after.push_back(std::make_unique<RenderSubmission>(render_pipeline));
  }

  EntityWorld::~EntityWorld()
  {
    CleanUp();
  }

  void EntityWorld::AddSystem(std::unique_ptr<EntitySystem> system)
  {
    _added.push_back(std::move(system));
  }

  void EntityWorld::RegisterComponents() const
  {
    _store->Register<Transform>("Transform");
    _store->Register<Camera>("Camera");
    _store->Register<Light>("Light");
    _store->Register<Spectator>("Spectator");

    // the renderer holds a model, textures, and a material for every entity
    // that was drawn, which are released when the entity stops being visible
    _store->Register<Renderable>("Renderable", [this](Entity, Renderable &renderable)
    {
      if (renderable.render_object_id < 0) { return; }

      _render_pipeline->DestroyRenderObject(renderable.render_object_id);
      renderable.render_object_id = -1;
    });
  }

  void EntityWorld::Initialize()
  {
    _logger->Info("Initializing the world");

    _store->Initialize();
    RegisterComponents();

    for (const auto &system : _before) { system->Initialize(*_store); }
    for (const auto &system : _added) { system->Initialize(*_store); }
    for (const auto &system : _after) { system->Initialize(*_store); }

    _scene->Populate(*_store);
    _initialized = true;

    _logger->Info("Initialized the world!");
    _input_context->CenterAndHideCursor();
  }

  void EntityWorld::Update()
  {
    const auto delta_time = _window_context->GetDeltaTime();

    for (const auto &system : _before) { system->Update(*_store, delta_time); }
    for (const auto &system : _added) { system->Update(*_store, delta_time); }
    for (const auto &system : _after) { system->Update(*_store, delta_time); }

    _render_pipeline->RenderFrame();
  }

  void EntityWorld::CleanUp()
  {
    if (!_initialized) { return; }
    _initialized = false;

    _logger->Info("Cleaning up the world");
    _store->CleanUp();
  }
} // neon
