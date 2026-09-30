#ifndef CSS_MEDIA_HPP
#define CSS_MEDIA_HPP

#include <string>
#include <vector>

// Media queries as https://www.w3.org/TR/mediaqueries-4/ defines them, with
// the features a game asks for.

namespace neon
{
  /// What a media query is asked against.
  struct CssEnvironment
  {
    /// The size of the window in points, which is what a pixel of CSS is.
    float width = 1920.0f;
    float height = 1080.0f;

    /// Pixels for each point: the density of the display.
    float resolution = 1.0f;

    /// Whether the player asked for less motion. The game says so.
    bool reduced_motion = false;

    bool operator==(const CssEnvironment &other) const = default;
  };

  /// One feature in brackets, such as `(min-width: 800px)`.
  struct CssMediaFeature
  {
    enum class Compare
    {
      /// `(width)`: the feature is there and is not 0 or none.
      Has = 0,
      Equal,
      AtLeast,
      AtMost,
      Above,
      Below
    };

    /// Without `min-` and `max-` in front.
    std::string name;

    Compare compare = Compare::Has;

    /// In points, in pixels for each point, or a ratio.
    float value = 0.0f;

    /// For a feature that holds a word, such as `orientation`.
    std::string keyword;
  };

  /// Features that all have to hold: `screen and (min-width: 800px)`.
  struct CssMediaQuery
  {
    bool is_negated = false;

    /// `all`, `screen`, or `print`. Empty counts as `all`.
    std::string type;

    std::vector<CssMediaFeature> features;
  };

  /// Queries of which one has to hold, set apart by commas.
  struct CssMediaQueryList
  {
    std::vector<CssMediaQuery> queries;

    /// As it was written.
    std::string text;

    /// A list without queries holds always.
    [[nodiscard]] bool Matches(const CssEnvironment &environment) const;
  };

  /// Reads what follows `@media`. Returns false and says why when it cannot
  /// be read.
  [[nodiscard]] bool ParseCssMedia(const std::string &text, CssMediaQueryList &list, std::string &error);
} // neon

#endif //CSS_MEDIA_HPP
