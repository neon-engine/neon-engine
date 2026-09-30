#ifndef CSS_STYLE_SHEET_HPP
#define CSS_STYLE_SHEET_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "css-media.hpp"
#include "css-selector.hpp"

// A style sheet as https://www.w3.org/TR/css-syntax-3/ defines it, read
// from text. What a property holds is not looked at here: a declaration is
// a name and a value, both as they were written.

namespace neon
{
  /// `name: value`, as it is written in a rule.
  struct CssDeclaration
  {
    /// As CSS writes it, with hyphens and in small letters. The name of a
    /// custom property starts with two hyphens and keeps its letters.
    std::string name;

    /// As it was written, without `!important` and without comments.
    std::string value;

    bool is_important = false;

    /// The line the declaration starts at, counted from 1.
    std::size_t line = 0;

    [[nodiscard]] bool IsCustomProperty() const
    {
      return name.starts_with("--");
    }
  };

  /// Selectors, and the declarations that hold for what they match.
  struct CssRule
  {
    std::vector<CssComplexSelector> selectors;

    /// The selectors as they were written, for messages.
    std::string selector_text;

    std::vector<CssDeclaration> declarations;
    std::size_t line = 0;

    /// The conditions of the `@media` the rule is inside of, as places in
    /// the list of the sheet. All of them have to hold.
    std::vector<std::size_t> media;
  };

  /// A block of `@keyframes`: what holds at a part of the way.
  struct CssKeyframe
  {
    /// From 0 at the start to 1 at the end.
    float offset = 0.0f;

    std::vector<CssDeclaration> declarations;
    std::size_t line = 0;
  };

  /// `@keyframes name { ... }`.
  struct CssKeyframes
  {
    std::string name;

    /// In rising order. Two with the same offset are in the order they
    /// were written in.
    std::vector<CssKeyframe> frames;

    std::size_t line = 0;
    std::vector<std::size_t> media;
  };

  /// `@font-face { ... }`.
  struct CssFontFace
  {
    std::string family;

    /// As it was written in `url()`.
    std::string source;

    int weight = 400;
    std::size_t line = 0;
  };

  /// `@import "other.css";`
  struct CssImport
  {
    /// As it was written.
    std::string path;

    /// The place of its condition in the list of the sheet, or -1 when it
    /// has none.
    int media = -1;

    std::size_t line = 0;
  };

  struct CssStyleSheet
  {
    /// What the sheet is called in messages, such as its virtual path.
    std::string path;

    std::vector<CssImport> imports;
    std::vector<CssFontFace> fonts;
    std::vector<CssRule> rules;
    std::vector<CssKeyframes> keyframes;

    /// The conditions of every `@media` and `@import` of the sheet.
    std::vector<CssMediaQueryList> media;
  };

  /// Reads a style sheet. What cannot be read is left out and the rest is
  /// kept, as a browser does. Every problem is added to `problems`, with
  /// the path of the sheet, the line, and what was expected.
  void ParseCssStyleSheet(
    const std::string &text,
    const std::string &path,
    CssStyleSheet &sheet,
    std::vector<std::string> &problems);

  /// Reads declarations alone, as they are written between the brackets of
  /// a rule and in the attribute `style` of HTML.
  void ParseCssDeclarations(
    const std::string &text,
    const std::string &path,
    std::size_t first_line,
    std::vector<CssDeclaration> &declarations,
    std::vector<std::string> &problems);
} // neon

#endif //CSS_STYLE_SHEET_HPP
