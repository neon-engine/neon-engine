#ifndef UI_RADIO_HPP
#define UI_RADIO_HPP

#include <neon/ui/ui-element.hpp>
#include <neon/ui/ui-text.hpp>

namespace neon
{
  /// One of several choices, of which one is chosen at a time. The radios
  /// of one `group` in one file belong together: choosing one lets go of
  /// the others.
  ///
  ///     - {type: radio, group: quality, value: low, text: Low, checked: "{quality}"}
  ///     - {type: radio, group: quality, value: high, text: High, checked: "{quality}"}
  ///
  /// `checked` is true, false, or a value of the game in brackets: the
  /// radio is chosen while that value is its `value`, and choosing it sets
  /// the value. Every radio of a group reports `changed` with `true` or
  /// `false` when it is chosen or let go of. The circle is the part
  /// `box`, and the dot in it the part `mark`.
  class UiRadio final : public UiElement
  {
    UiText _text;
    UiFlag _enabled{true};
    bool _is_enabled = true;
    bool _autofocus = false;

    std::string _group;
    std::string _value;
    bool _checked = false;
    std::string _binding;
    bool _has_followed = false;
    std::uint64_t _followed_revision = 0;

    /// Lets go of the others of the group, from the root of the tree.
    void LetGoOfTheOthers();

  public:
    static constexpr const char *kType = "radio";

    void ApplyDefaults(UiStyle &style) const override;

    void ApplyStateDefaults(UiStyle &style, const UiStates &states) const override;

    void ApplyPartDefaults(const std::string &part, UiStyle &style) const override;

    void ReadAttributes(const DataReader &reader) override;

    [[nodiscard]] bool IsFocusable() const override;

    [[nodiscard]] bool IsClickable() const override;

    [[nodiscard]] bool WantsFocus() const override;

    [[nodiscard]] bool IsEnabled() const override;

    [[nodiscard]] bool IsChecked() const override;

    [[nodiscard]] bool HasContent() const override;

    [[nodiscard]] bool TellsWhenItChanged() const override;

    [[nodiscard]] LayoutSize Measure(
      const UiFrame &frame,
      float available_width,
      float available_height) override;

    void Update(const UiFrame &frame) override;

    void Interact(UiInteraction &interaction, const UiFrame &frame) override;

    void PaintContent(
      UiPainter &painter,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity) override;

    [[nodiscard]] bool GetField(const std::string &name, FieldValue &value) const override;

    bool SetField(const std::string &name, const FieldValue &value, std::string &error) override;

    [[nodiscard]] std::vector<Field> GetFields() const override;

    /// Chooses it, and lets go of the others of its group. Choosing what
    /// is chosen changes nothing.
    void Choose();

    /// Lets go of it without choosing another, as the game does when it
    /// sets the value to something none of the group has.
    void LetGo();

    [[nodiscard]] const std::string &GetGroup() const;

    [[nodiscard]] const std::string &GetValue() const;

    [[nodiscard]] const std::string &GetText() const;
  };
} // neon

#endif //UI_RADIO_HPP
