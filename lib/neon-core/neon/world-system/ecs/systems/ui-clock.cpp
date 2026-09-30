#include "ui-clock.hpp"

namespace neon
{
  UiClock::UiClock(UiContext *ui_context)
  {
    _ui_context = ui_context;
  }

  void UiClock::Initialize(EntityStore &store) {}

  void UiClock::Update(EntityStore &store, const double delta_time)
  {
    _ui_context->AdvanceTime(delta_time);
  }
} // neon
