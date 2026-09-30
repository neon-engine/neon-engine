#ifndef UI_STYLE_SHEETS_HPP
#define UI_STYLE_SHEETS_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <neon/filesystem/file-system-context.hpp>

#include "css/css-style-sheet.hpp"

namespace neon
{
  /// The style sheets of a user interface, read from files and put in the
  /// order the cascade asks for.
  ///
  /// Sheets are named by virtual paths. A path without a scheme is looked
  /// for next to what names it: a sheet that a file of YAML names next to
  /// that file, and a sheet that `@import` names next to the sheet it is
  /// written in.
  class UiStyleSheets
  {
  public:
    /// One selector with the declarations of its rule.
    struct Rule
    {
      CssComplexSelector selector;
      std::shared_ptr<const std::vector<CssDeclaration>> declarations;

      /// The path of the sheet, and what the rule is called in a message.
      std::string sheet;
      std::string where;

      /// The conditions the rule is under, as places in the list of all
      /// conditions. All of them have to hold.
      std::vector<std::size_t> media;

      /// The place among all rules, in the order CSS gives them.
      std::size_t order = 0;
    };

    struct Keyframes
    {
      std::string name;
      std::vector<CssKeyframe> frames;
      std::string sheet;
      std::vector<std::size_t> media;
    };

    struct Font
    {
      std::string family;
      int weight = 400;

      /// The virtual path of the font.
      std::string path;
    };

  private:
    std::vector<std::string> _paths;
    std::vector<Rule> _rules;
    std::vector<Keyframes> _keyframes;
    std::vector<Font> _fonts;
    CssDependencies _dependencies;

    std::vector<CssMediaQueryList> _media;
    std::vector<bool> _media_holds;
    CssEnvironment _environment;

    // the sheets that are being read, to tell a circle of imports
    std::vector<std::string> _reading;

    bool ReadSheet(
      const std::string &path,
      const std::vector<std::size_t> &media,
      FileSystemContext &file_system,
      std::vector<std::string> &errors,
      std::vector<std::string> &warnings,
      const std::string &named_by);

    void Add(
      const CssStyleSheet &sheet,
      const std::vector<std::size_t> &media,
      const std::vector<std::size_t> &own_media,
      std::vector<std::string> &warnings);

  public:
    /// Reads the sheets at the paths, in their order. `base` is the path of
    /// what names them. Returns false when a sheet cannot be read at all,
    /// with a message in `errors`. What is wrong inside a sheet is left out
    /// and said in `warnings`, and the rest of the sheet is used.
    bool Load(
      const std::vector<std::string> &paths,
      const std::string &base,
      FileSystemContext &file_system,
      std::vector<std::string> &errors,
      std::vector<std::string> &warnings);

    /// Takes a sheet from text, behind the sheets that are there. `path` is
    /// what it is called in messages. Imports are not followed.
    void AddText(const std::string &text, const std::string &path, std::vector<std::string> &warnings);

    /// The paths Load() was given, as they were resolved.
    [[nodiscard]] const std::vector<std::string> &GetPaths() const;

    [[nodiscard]] bool IsEmpty() const;

    /// Every rule, in the order of the cascade: sheets in the order they
    /// are named in, what a sheet imports in front of its own rules.
    [[nodiscard]] const std::vector<Rule> &GetRules() const;

    /// Whether every condition of `@media` a rule is under holds.
    [[nodiscard]] bool Holds(const std::vector<std::size_t> &media) const;

    /// The keyframes of a name whose conditions hold, or nullptr. Of
    /// several, the one that was written last.
    [[nodiscard]] const Keyframes *FindKeyframes(const std::string &name) const;

    [[nodiscard]] const std::vector<Font> &GetFonts() const;

    /// What the selectors depend on next to the element they are about.
    [[nodiscard]] const CssDependencies &GetDependencies() const;

    /// Says what the conditions of `@media` are asked against. Returns
    /// whether one of them holds now that did not, or the other way
    /// around, which changes the styles of everything.
    bool SetEnvironment(const CssEnvironment &environment);

    [[nodiscard]] const CssEnvironment &GetEnvironment() const;
  };

  /// The virtual path of what is named next to something else: `theme.css`
  /// next to `assets://ui/menu.ui.yml` is `assets://ui/theme.css`. A path
  /// with a scheme is what it is. `..` goes up a folder, and not above the
  /// scheme.
  [[nodiscard]] std::string ResolveUiPath(const std::string &base, const std::string &path);
} // neon

#endif //UI_STYLE_SHEETS_HPP
