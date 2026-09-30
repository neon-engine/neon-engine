#ifndef UI_CHECKBOX_HPP
#define UI_CHECKBOX_HPP

#include <neon/ui/ui-element.hpp>
#include <neon/ui/ui-text.hpp>

namespace neon
{
  /// A box that is ticked or not, with a text next to it. It is what a
  /// setting that is on or off is chosen with.
  ///
  ///     type: checkbox
  ///     name: fullscreen
  ///     text: Full screen
  ///     checked: "{fullscreen}"
  ///
  /// `checked` is true, false, or a value of the game in brackets, which
  /// ticking the box changes. It reports `changed` with `true` or `false`.
  /// The box is the part `box`, and the tick in it the part `mark`; a
  /// style sheet reaches them as `checkbox::box` and `checkbox::mark`, and
  /// asks for the ticked one with `checkbox:checked`.
  class UiCheckbox : public UiElement
  {
    UiText _text;
    UiFlag _enabled{true};
    bool _is_enabled = true;
    bool _autofocus = false;

    bool _checked = false;
    std::string _binding;
    bool _has_followed = false;
    std::uint64_t _followed_revision = 0;

  protected:
    /// How large the box is, in units of the file.
    [[nodiscard]] virtual float BoxWidth() const;

    [[nodiscard]] virtual float BoxHeight() const;

    /// Draws the box, as ticked or not, into its place.
    virtual void PaintBox(UiPainter &painter, const UiFrame &frame, const UiRectangle &box, float opacity);

    /// Ticks the box, or takes the tick away, and tells the game.
    void SetChecked(bool checked);

  public:
    static constexpr const char *kType = "checkbox";

    UiCheckbox();

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

    [[nodiscard]] const std::string &GetText() const;
  };

  /// A checkbox drawn as a switch: a knob that slides to the right when it
  /// is on. Left and right move it as well.
  ///
  ///     type: toggle
  ///     name: vsync
  ///     text: V-Sync
  ///     checked: true
  ///
  /// The switch is the part `track`, and the knob the part `thumb`.
  class UiToggle final : public UiCheckbox
  {
  protected:
    [[nodiscard]] float BoxWidth() const override;

    [[nodiscard]] float BoxHeight() const override;

    void PaintBox(UiPainter &painter, const UiFrame &frame, const UiRectangle &box, float opacity) override;

  public:
    static constexpr const char *kType = "toggle";

    void ApplyPartDefaults(const std::string &part, UiStyle &style) const override;

    [[nodiscard]] bool UsesDirection(Key direction) const override;

    void Interact(UiInteraction &interaction, const UiFrame &frame) override;
  };
} // neon

#endif //UI_CHECKBOX_HPP
