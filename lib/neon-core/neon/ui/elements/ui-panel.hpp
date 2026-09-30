#ifndef UI_PANEL_HPP
#define UI_PANEL_HPP

#include <neon/ui/ui-element.hpp>

namespace neon
{
  /// A box that holds other elements, and may have a background and a
  /// border. It is what a `div` is in HTML.
  ///
  ///     type: panel
  ///     background_color: "#00000080"
  ///     padding: 16
  ///     children:
  ///       - type: label
  ///         text: Paused
  class UiPanel final : public UiElement
  {
  public:
    static constexpr const char *kType = "panel";

    /// The pointer goes through a panel to the game, unless the file says
    /// `pointer_events: auto`.
    void ApplyDefaults(UiStyle &style) const override;

    [[nodiscard]] bool TakesChildren() const override;
  };
} // neon

#endif //UI_PANEL_HPP
