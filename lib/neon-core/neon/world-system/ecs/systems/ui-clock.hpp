#ifndef UI_CLOCK_HPP
#define UI_CLOCK_HPP

#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Moves the time of the user interface on by what the world advances
  /// by. The world is what asks the window for the time of a frame, and it
  /// is asked once in a frame.
  ///
  ///     world.AddSystem(std::make_unique<neon::UiClock>(&ui_system));
  class UiClock final : public EntitySystem
  {
    UiContext *_ui_context;

  public:
    explicit UiClock(UiContext *ui_context);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //UI_CLOCK_HPP
