#ifndef UI_LABEL_HPP
#define UI_LABEL_HPP

#include <neon/ui/ui-element.hpp>
#include <neon/ui/ui-text.hpp>

namespace neon
{
  /// A text. It is as large as the text, unless the file gives it a size.
  ///
  ///     type: label
  ///     text: "Health: {health}"
  ///     font_size: 24
  ///     color: "#ffffff"
  class UiLabel final : public UiElement
  {
    UiText _text;

  public:
    static constexpr const char *kType = "label";

    void ApplyDefaults(UiStyle &style) const override;

    void ReadAttributes(const DataReader &reader) override;

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

#endif //UI_LABEL_HPP
