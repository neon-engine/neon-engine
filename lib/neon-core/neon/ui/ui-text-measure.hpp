#ifndef UI_TEXT_MEASURE_HPP
#define UI_TEXT_MEASURE_HPP

#include <cstddef>
#include <string>
#include <vector>

#include <neon/layout/layout-engine.hpp>
#include <neon/text/text-layout.hpp>

#include "ui-resources.hpp"

namespace neon
{
  /// Where the characters of a text are once it is laid out, for a caret
  /// and for a click into the text.
  ///
  /// It is an interface, so that what places characters can change
  /// underneath it: today the characters of a font are placed one behind
  /// the other, and a caret goes between two of them. Once text is shaped,
  /// a caret goes between the clusters of glyphs that shaping comes to,
  /// which an implementation of this on the shaping answers with.
  class UiTextMeasure
  {
  public:
    /// A text as it is to be laid out: with the font, and with what
    /// PlaceText() is given.
    struct Request
    {
      const UiFont *font = nullptr;

      /// UTF-8.
      std::string text;

      TextOptions options;

      /// Whether a word that is wider than `max_width` is broken where it
      /// no longer fits, as `overflow-wrap: anywhere` of CSS does. A text
      /// that is typed into asks for it, so that nothing is out of sight.
      /// PlaceText() does not do this: what asks for it draws line by
      /// line.
      bool breaks_long_words = false;
    };

    /// Where a caret stands, in pixels from the left top corner of the
    /// text.
    struct Caret
    {
      float x = 0.0f;
      float y = 0.0f;
      float height = 0.0f;
      std::size_t line = 0;
    };

    /// A line of the text: the bytes it is made of, without what ends
    /// it, and where it is.
    struct Line
    {
      std::size_t start = 0;
      std::size_t end = 0;
      float top = 0.0f;
      float height = 0.0f;
      float width = 0.0f;
    };

  protected:
    ~UiTextMeasure() = default;

  public:
    /// Where a caret stands in front of the byte at a place. At the end
    /// of the text, behind its last character.
    [[nodiscard]] virtual Caret CaretAt(const Request &request, std::size_t offset) const = 0;

    /// The place in the text a caret goes to when the text is pointed at,
    /// in pixels from the left top corner of the text: the nearest place
    /// a caret may stand at in the line that is pointed at.
    [[nodiscard]] virtual std::size_t OffsetAt(const Request &request, float x, float y) const = 0;

    /// The lines the text is laid out in, from the top.
    [[nodiscard]] virtual std::vector<Line> LinesOf(const Request &request) const = 0;

    /// The size of the text as PlaceText() sees it.
    [[nodiscard]] virtual LayoutSize SizeOf(const Request &request) const = 0;
  };

  /// Places the characters of the font one behind the other, as
  /// PlaceText() does, and answers where each of them is. It follows the
  /// rules of PlaceText() so that a caret lands between the characters as
  /// they are drawn: lines are broken at spaces, a line ends at a line
  /// feed, a tab is a space, and a carriage return and a soft hyphen take
  /// no room.
  // ReSharper disable once CppInconsistentNaming
  class Atlas_UiTextMeasure final : public UiTextMeasure
  {
  public:
    /// The one to use where nothing else is given.
    [[nodiscard]] static const Atlas_UiTextMeasure &Get();

    [[nodiscard]] Caret CaretAt(const Request &request, std::size_t offset) const override;

    [[nodiscard]] std::size_t OffsetAt(const Request &request, float x, float y) const override;

    [[nodiscard]] std::vector<Line> LinesOf(const Request &request) const override;

    [[nodiscard]] LayoutSize SizeOf(const Request &request) const override;
  };
} // neon

#endif //UI_TEXT_MEASURE_HPP
