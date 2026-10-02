#ifndef UI_SURFACE_LOADING_HPP
#define UI_SURFACE_LOADING_HPP

#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>
#include <neon/world-system/ecs/scene-file/component-format.hpp>

namespace neon
{
  /// Makes the surface of every entity that carries a `UiSurfaceView` and
  /// shows its user interface on it, and releases both when the entity or
  /// the component goes away.
  ///
  /// An application adds it to the world, together with the format of the
  /// component for its scene files:
  ///
  ///     scene.GetComponentFormats().Add(neon::UiSurfaceFormat());
  ///     world.AddSystem(std::make_unique<neon::UiSurfaceLoading>(&ui_system));
  class UiSurfaceLoading final : public EntitySystem
  {
    UiContext *_ui_context;
    QueryId _query = 0;

  public:
    /// What the component is called in a scene file.
    static constexpr const char *kComponent_Name = "UiSurface";

    explicit UiSurfaceLoading(UiContext *ui_context);

    /// Registers the component, which the engine does not know by itself.
    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    /// Throws when a surface cannot be made or a file cannot be used, as a
    /// scene does that cannot be read.
    void Update(EntityStore &store, double delta_time) override;
  };

  /// How a `UiSurfaceView` is written in a scene file.
  [[nodiscard]] ComponentFormat UiSurfaceFormat();
} // neon

#endif //UI_SURFACE_LOADING_HPP
