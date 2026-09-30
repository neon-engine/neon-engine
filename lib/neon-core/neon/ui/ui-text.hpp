#ifndef UI_TEXT_HPP
#define UI_TEXT_HPP

#include <cstdint>
#include <string>

#include <neon/data/data-reader.hpp>
#include <neon/layout/layout-engine.hpp>

#include "ui-element.hpp"

namespace neon
{
  /// The text of an element that has one, such as a label and a button. It
  /// follows the values of the game, and is measured and drawn in pixels of
  /// the frame, so that it is as sharp as the font was drawn.
  class UiText
  {
    UiTemplate _template;
    std::string _text;
    std::u32string _characters;
    bool _has_text = false;

    // the values the text was made from
    std::uint64_t _revision = 0;
    bool _is_made = false;

    [[nodiscard]] static const UiFont *FontOf(const UiStyle &style, const UiFrame &frame);

    [[nodiscard]] static TextOptions OptionsOf(const UiStyle &style, const UiFrame &frame);

  public:
    /// Reads `text` of the element. Text that is left out is empty.
    void Read(const DataReader &reader);

    /// Whether `text` was written.
    [[nodiscard]] bool IsWritten() const;

    /// The text as it is shown now.
    [[nodiscard]] const std::string &Get() const;

    /// Puts the values of the game into their places, when one of them has
    /// changed.
    void Update(const UiFrame &frame);

    /// In units of the file.
    [[nodiscard]] LayoutSize Measure(const UiStyle &style, const UiFrame &frame, float available_width) const;

    /// `centered` places the text in the middle of the height of the box,
    /// and not at its top.
    void Paint(
      UiPainter &painter,
      const UiStyle &style,
      const UiFrame &frame,
      const UiRectangle &content_box,
      float opacity,
      bool centered) const;
  };
} // neon

#endif //UI_TEXT_HPP
