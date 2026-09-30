#ifndef UI_VIEW_FORMAT_HPP
#define UI_VIEW_FORMAT_HPP

#include "component-format.hpp"

namespace neon
{
  /// How the component that shows a user interface is written in a scene
  /// file:
  ///
  ///     Ui:
  ///       file: assets://ui/hud.ui.yml
  ///
  /// The engine does not add it by itself, since a world without a user
  /// interface has no such component. An application that has one adds it
  /// next to the system UiViewLoading.
  [[nodiscard]] ComponentFormat UiViewFormat();
} // neon

#endif //UI_VIEW_FORMAT_HPP
