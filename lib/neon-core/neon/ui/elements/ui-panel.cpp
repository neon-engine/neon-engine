#include "ui-panel.hpp"

namespace neon
{
  void UiPanel::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::None;
  }

  bool UiPanel::TakesChildren() const
  {
    return true;
  }
} // neon
