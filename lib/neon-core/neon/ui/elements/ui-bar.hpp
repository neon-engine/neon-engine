#ifndef UI_BAR_HPP
#define UI_BAR_HPP

#include <neon/ui/ui-element.hpp>

namespace neon
{
  /// Shows how much of something there is, such as health. It is what a
  /// `progress` is in HTML: `value` of `max` is filled, from the left.
  ///
  ///     type: bar
  ///     value: "{health}"
  ///     max: 100
  ///     accent_color: "#e5484d"
  ///
  /// `background_color` is what is behind the filling, and `accent_color`
  /// the filling itself.
  class UiBar final : public UiElement
  {
    UiNumber _value{0.0};
    UiNumber _max{1.0};
    float _filled = 0.0f;

    // what the two came to when the values of the game were last followed
    float _shown_value = 0.0f;
    float _shown_max = 1.0f;

    static void ReadNumber(const DataReader &reader, const std::string &name, UiNumber &number);

  public:
    /// It says when what it shows changed.
    [[nodiscard]] bool TellsWhenItChanged() const override;

    [[nodiscard]] bool GetField(const std::string &name, FieldValue &value) const override;

    bool SetField(const std::string &name, const FieldValue &value, std::string &error) override;

    [[nodiscard]] std::vector<Field> GetFields() const override;

    static constexpr const char *kType = "bar";

    void ApplyDefaults(UiStyle &style) const override;

    void ReadAttributes(const DataReader &reader) override;

    void Update(const UiFrame &frame) override;

    void PaintContent(
      UiPainter &painter,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity) override;

    /// How much is filled, from 0 to 1.
    [[nodiscard]] float GetFilled() const;
  };
} // neon

#endif //UI_BAR_HPP
