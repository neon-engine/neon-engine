#include "entity-world.hpp"

#include <neon/common/transform.hpp>

#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/persistent.hpp"
#include "components/player.hpp"
#include "components/renderable.hpp"
#include "components/scene-exit.hpp"
#include "components/trigger.hpp"
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

  void EntityWorld::AddSystemAfterPlacing(std::unique_ptr<EntitySystem> system)
  {
    _added_after_placing.push_back(std::move(system));
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
    _store->Register<Player>("Player");

    // the renderer holds a model, textures, and a material for every entity
    // that was drawn, which are released when the entity stops being visible
    _store->Register<Persistent>("Persistent");
    _store->Register<SceneExit>("SceneExit");
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

    // every component first, so that a system finds the components of
    // another when it makes its queries, whichever comes first
    for (const auto &system : _before) { system->Register(*_store); }
    for (const auto &system : _added) { system->Register(*_store); }
    for (const auto &system : _placing) { system->Register(*_store); }
    for (const auto &system : _after) { system->Register(*_store); }
    for (const auto &system : _added_after_placing) { system->Register(*_store); }

    for (const auto &system : _before) { system->Initialize(*_store); }
    for (const auto &system : _added) { system->Initialize(*_store); }
    for (const auto &system : _placing) { system->Initialize(*_store); }
    for (const auto &system : _after) { system->Initialize(*_store); }
    for (const auto &system : _added_after_placing) { system->Initialize(*_store); }
    _exits = _store->Query<SceneExit>();

    // a scene with problems is said in the log, and the world runs with
    // what could be read: a game is not ended by a file
    if (!_scene->Populate(*_store))
    {
      _logger->Error("The world runs with what could be read of its scene");
    }
    _initialized = true;

    _logger->Info("Initialized the world!");
    _input_context->CenterAndHideCursor();
  }

  void EntityWorld::LoadScene(const std::string &file_path)
  {
    if (file_path.empty())
    {
      _logger->Error("A scene without a path was asked for, nothing changes");
      return;
    }
    _scene_to_load = file_path;
  }

  bool EntityWorld::IsChangingScene() const
  {
    return !_scene_to_load.empty();
  }

  void EntityWorld::ChangeScene()
  {
    const std::string path = _scene_to_load;
    _scene_to_load.clear();
    _logger->Info("Changing the scene to {}", path);

    // what stays is what carries a Persistent, with everything below it;
    // the rest goes, which releases what the systems hold for it
    for (const Entity entity : _store->GetChildren(No_Entity))
    {
      if (_store->Has<Persistent>(entity)) { continue; }
      _store->DestroyEntity(entity);
    }

    if (!_scene->Load(*_store, path))
    {
      _logger->Error("The world runs with what could be read of the scene {}", path);
    }
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
    if (!_scene_to_load.empty()) { ChangeScene(); }

    const auto delta_time = _window_context->GetDeltaTime();

    if (_paused)
    {
      // nothing moves and no time passes, not for the steps either, and
      // what is there is drawn as it was
      for (const auto &system : _placing) { system->Update(*_store, 0.0); }
      for (const auto &system : _after) { system->Update(*_store, 0.0); }
      for (const auto &system : _added_after_placing) { system->Update(*_store, 0.0); }
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
    for (const auto &system : _added_after_placing) { system->Update(*_store, delta_time); }

    CheckExits();
    _render_pipeline->RenderFrame();
  }

  void EntityWorld::CheckExits()
  {
    // The physics registers Trigger, when there is one; it is looked up by
    // name so that a world without physics has exits that never open
    const ComponentId trigger_id = _store->FindComponent("Trigger");
    if (trigger_id == No_Component) { return; }

    std::string scene;
    _store->Each(_exits, [this, trigger_id, &scene](const EntityBlock &block)
    {
      const auto *exits = block.Column<SceneExit>(0);
      for (std::size_t i = 0; i < block.count; i++)
      {
        const auto *trigger = static_cast<const Trigger *>(_store->GetComponent(block.entities[i], trigger_id));
        if (trigger == nullptr || trigger->inside == 0 || exits[i].scene.empty()) { continue; }
        scene = exits[i].scene;
      }
    });

    if (!scene.empty()) { LoadScene(scene); }
  }

  void EntityWorld::CleanUp()
  {
    if (!_initialized) { return; }
    _initialized = false;

    _logger->Info("Cleaning up the world");
    _store->CleanUp();
  }
} // neon
