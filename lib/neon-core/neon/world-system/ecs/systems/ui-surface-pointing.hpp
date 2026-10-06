#ifndef UI_SURFACE_POINTING_HPP
#define UI_SURFACE_POINTING_HPP

#include <glm/glm.hpp>

#include <neon/common/transform.hpp>
#include <neon/input/input-context.hpp>
#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Finds where the player points on the screens in the world, and tells
  /// the user interface, so that a terminal is used by walking up to it.
  ///
  /// The ray starts at the camera that draws the window and goes where it
  /// looks, which is where the dot in the middle of a HUD is. A screen is
  /// the square of 1 by 1 of its entity, facing +Z, as the quad model is.
  /// It is pointed at from its front, and within `reach` of the camera.
  /// The nearest screen the ray hits gets the pointer, with the button of
  /// the pointer or accept as its press, and the flag `pointing` tells the
  /// files on the window whether anything is pointed at. The screen is
  /// also told who points at it, `pointed_by` of its UiSurface: the nearest
  /// entity from the camera up that carries a Player, or the camera's own.
  ///
  /// An application adds it after UiSurfaceLoading, with the input as the
  /// game sees it:
  ///
  ///     world.AddSystem(std::make_unique<neon::UiSurfacePointing>(&ui_system, ui_system.GetGameInput()));
  class UiSurfacePointing final : public EntitySystem
  {
    UiContext *_ui_context;
    InputContext *_input_context;
    QueryId _cameras = 0;
    QueryId _surfaces = 0;

    // the component that says who a camera belongs to, when there is one
    ComponentId _player = No_Component;

    // the surface that had the pointer in the frame before
    int _pointed = No_Ui_Surface;

  public:
    UiSurfacePointing(UiContext *ui_context, InputContext *input_context);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;

    /// Where a ray hits the square of an entity, if it does: `u` and `v`
    /// on the surface from its left top corner, from 0 to 1, and how far
    /// along the ray, in units of the world for a direction of length 1.
    /// The square is hit from its front only.
    [[nodiscard]] static bool Hit(
      const Transform &transform,
      const glm::vec3 &origin,
      const glm::vec3 &direction,
      float &u,
      float &v,
      float &distance);
  };
} // neon

#endif //UI_SURFACE_POINTING_HPP
