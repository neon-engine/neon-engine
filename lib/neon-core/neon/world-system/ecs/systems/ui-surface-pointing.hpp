#ifndef UI_SURFACE_POINTING_HPP
#define UI_SURFACE_POINTING_HPP

#include <vector>

#include <neon/input/input-context.hpp>
#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Puts the pointer on the screens in the world that rays hit, so that
  /// a terminal is used by walking up to it and looking at it.
  ///
  /// It reads the `RayCast` of every entity that has one, which the
  /// RayCasting system fills: a ray that hits a screen gives that screen
  /// the pointer, at the hit, with the button of the pointer or accept as
  /// its press, unless the hit is further than the screen's `reach`. The
  /// screen is told who points at it, `pointed_by` of its UiSurface: the
  /// `owner` the ray names, or the ray's own entity. Nothing here knows
  /// what entity the ray hangs under, and nothing here tells a HUD what to
  /// show: a script of the game reads the ray for that.
  ///
  /// An application adds it after RayCasting and UiSurfaceLoading, with
  /// the input as the game sees it:
  ///
  ///     world.AddSystem(std::make_unique<neon::UiSurfacePointing>(&ui_system, ui_system.GetGameInput()));
  class UiSurfacePointing final : public EntitySystem
  {
    UiContext *_ui_context;
    InputContext *_input_context;
    QueryId _rays = 0;

    // the surfaces that had a pointer in the frame before
    std::vector<int> _pointed;

  public:
    UiSurfacePointing(UiContext *ui_context, InputContext *input_context);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //UI_SURFACE_POINTING_HPP
