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
    _placing.push_back(std::make_unique<TransformPropagation>());
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

  FixedClock &EntityWorld::GetFixedClock()
  {
    return _fixed_clock;
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
    for (const auto &system : _placing) { system->Initialize(*_store); }
    for (const auto &system : _after) { system->Initialize(*_store); }

    _scene->Populate(*_store);
    _initialized = true;

    _logger->Info("Initialized the world!");
    _input_context->CenterAndHideCursor();
  }

  void EntityWorld::SetPaused(const bool paused)
  {
    _paused = paused;
  }

  bool EntityWorld::IsPaused() const
  {
    return _paused;
  }

  void EntityWorld::Update()
  {
    const auto delta_time = _window_context->GetDeltaTime();

    if (_paused)
    {
      // nothing moves and no time passes, not for the steps either, and
      // what is there is drawn as it was
      for (const auto &system : _placing) { system->Update(*_store, 0.0); }
      for (const auto &system : _after) { system->Update(*_store, 0.0); }
      _render_pipeline->RenderFrame();
      return;
    }

    // the steps come first, so that the frame sees what they led to
    const auto steps = _fixed_clock.Advance(delta_time);
    const auto fixed_delta_time = _fixed_clock.GetStep();

    for (std::size_t step = 0; step < steps; step++)
    {
      for (const auto &system : _before) { system->FixedUpdate(*_store, fixed_delta_time); }
      for (const auto &system : _added) { system->FixedUpdate(*_store, fixed_delta_time); }
    }

    for (const auto &system : _before) { system->Update(*_store, delta_time); }
    for (const auto &system : _added) { system->Update(*_store, delta_time); }
    for (const auto &system : _placing) { system->Update(*_store, delta_time); }

    // what is drawn lies between the last two steps
    const auto blend = _fixed_clock.GetBlend();
    for (const auto &system : _before) { system->Interpolate(*_store, blend); }
    for (const auto &system : _added) { system->Interpolate(*_store, blend); }

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
