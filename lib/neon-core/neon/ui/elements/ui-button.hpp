#ifndef UI_BUTTON_HPP
#define UI_BUTTON_HPP

#include <neon/ui/ui-element.hpp>
#include <neon/ui/ui-text.hpp>

namespace neon
{
  /// What a player chooses with the pointer, the keys, or a controller. It
  /// reports a click under its name.
  ///
  ///     type: button
  ///     name: start
  ///     text: Start
  ///     autofocus: true
  ///     hover:
  ///       background_color: "#4c566a"
  ///
  /// It holds a text, or other elements, such as an image next to a label.
  /// With `action: close` choosing it also closes its file, as the Back
  /// button of a menu does, so a menu can be left without code of the game.
  class UiButton final : public UiElement
  {
    UiText _text;
    UiFlag _enabled{true};
    bool _is_enabled = true;
    bool _autofocus = false;
    bool _closes = false;

  public:
    /// It says when what it shows changed.
    [[nodiscard]] bool TellsWhenItChanged() const override;

    [[nodiscard]] bool GetField(const std::string &name, FieldValue &value) const override;

    bool SetField(const std::string &name, const FieldValue &value, std::string &error) override;

    [[nodiscard]] std::vector<Field> GetFields() const override;

    static constexpr const char *kType = "button";

    void ApplyDefaults(UiStyle &style) const override;

    void ApplyStateDefaults(UiStyle &style, const UiStates &states) const override;

    void ReadAttributes(const DataReader &reader) override;

    [[nodiscard]] bool TakesChildren() const override;

    [[nodiscard]] bool IsFocusable() const override;

    [[nodiscard]] bool IsClickable() const override;

    [[nodiscard]] bool ClosesItsFile() const override;

    [[nodiscard]] bool WantsFocus() const override;

    [[nodiscard]] bool IsEnabled() const override;

    [[nodiscard]] bool HasContent() const override;

    [[nodiscard]] LayoutSize Measure(
      const UiFrame &frame,
      float available_width,
      float available_height) override;

    void Update(const UiFrame &frame) override;

    void PaintContent(
      UiPainter &painter,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity) override;

    /// The text as it is shown now.
    [[nodiscard]] const std::string &GetText() const;
  };
} // neon

#endif //UI_BUTTON_HPP
