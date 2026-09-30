#ifndef UI_VIEW_HPP
#define UI_VIEW_HPP

#include <string>

namespace neon
{
  /// Shows a user interface for as long as the entity that carries it is
  /// there. It is how a scene names the user interface it comes with.
  ///
  ///     Ui:
  ///       file: assets://ui/hud.ui.yml
  struct UiView
  {
    /// Virtual path of the file.
    std::string file;

    /// What the user interface knows the file as once it is shown. -1
    /// before that.
    int document = -1;

    /// Whether showing it was tried, so that a file that cannot be used is
    /// not read again in every frame.
    bool is_tried = false;
  };
} // neon

#endif //UI_VIEW_HPP
