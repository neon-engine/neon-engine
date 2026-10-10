#ifndef UI_SLIDER_HPP
#define UI_SLIDER_HPP

#include <neon/ui/ui-element.hpp>

namespace neon
{
  /// A number between two others, chosen by moving a knob along a track:
  /// a volume, a sensitivity.
  ///
  ///     type: slider
  ///     name: volume
  ///     min: 0
  ///     max: 100
  ///     step: 5
  ///     value: "{volume}"
  ///
  /// `value` is a number, or a value of the game in brackets, which moving
  /// the knob sets. The pointer drags the knob or puts it where it
  /// clicked; left and right move it by a step, page up and down by ten,
  /// and home and end to the ends. It reports `changed` with the number.
  /// With `on_change: tune` moving it calls a function of the scripts.
  /// The track is the part `track`, what is filled up to the knob the
  /// part `fill`, and the knob the part `thumb`.
  class UiSlider final : public UiElement
  {
    UiFlag _enabled{true};
    bool _is_enabled = true;
    bool _autofocus = false;

    double _min = 0.0;
    double _max = 100.0;
    double _step = 1.0;
    double _value = 0.0;
    std::string _binding;
    bool _has_followed = false;
    std::uint64_t _followed_revision = 0;

    bool _is_dragging = false;

    // the function `on_change` names, with what it is handed
    UiCall _on_change;

    /// Rounds to a step from the least, and keeps it between the ends.
    [[nodiscard]] double Snapped(double value) const;

    /// The value at a place of the pointer, in units of the file.
    [[nodiscard]] double ValueAt(float x) const;

    /// Sets it and tells the game when it changed. `by_player` says that
    /// the player moved it, and not the game.
    void Change(double value, bool by_player);

  public:
    static constexpr const char *kType = "slider";

    void ApplyDefaults(UiStyle &style) const override;

    void ApplyStateDefaults(UiStyle &style, const UiStates &states) const override;

    void ApplyPartDefaults(const std::string &part, UiStyle &style) const override;

    void ReadAttributes(const DataReader &reader) override;

    [[nodiscard]] bool IsFocusable() const override;

    [[nodiscard]] bool WantsFocus() const override;

    [[nodiscard]] bool IsEnabled() const override;

    [[nodiscard]] bool UsesDirection(Key direction) const override;

    [[nodiscard]] bool HasContent() const override;

    [[nodiscard]] bool TellsWhenItChanged() const override;

    [[nodiscard]] const UiCall *CallsWhenChanged() const override;

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

    [[nodiscard]] double GetValue() const;

    /// Where the value is between the ends, from 0 to 1.
    [[nodiscard]] float GetFraction() const;
  };
} // neon

#endif //UI_SLIDER_HPP
