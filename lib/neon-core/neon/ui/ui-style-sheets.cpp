#include "ui-style-sheets.hpp"

#include <algorithm>
#include <format>

#include "ui-declarations.hpp"

namespace neon
{
  // Helpers of UiStyleSheets, for this file alone.
  namespace
  {
    /// The line a message names behind the path of its sheet.
    std::string WithLine(const std::string &path, const std::size_t line, const std::string &message)
    {
      return std::format("{}:{}: {}", path, line, message);
    }

    /// Leaves out the declarations that cannot be read, and says so.
    void Check(
      std::vector<CssDeclaration> &declarations,
      const std::string &path,
      const std::string &where,
      std::vector<std::string> &warnings)
    {
      std::erase_if(declarations, [&](const CssDeclaration &declaration)
      {
        if (declaration.IsCustomProperty()) { return false; }

        // how long the step between two keyframes takes its time
        std::vector<std::string> problems;
        if (CheckUiDeclaration(declaration.name, declaration.value, where, problems)) { return false; }

        for (const auto &problem : problems)
        {
          warnings.push_back(WithLine(path, declaration.line, problem + ". The declaration is left out"));
        }
        return true;
      });
    }

    /// Writes the paths in `url()` that stand alone as virtual paths, so
    /// that a value says where its file is without the sheet it is from.
    void ResolveUrls(std::vector<CssDeclaration> &declarations, const std::string &sheet)
    {
      for (auto &declaration : declarations)
      {
        std::string &value = declaration.value;
        if (value.size() < 6 || !value.starts_with("url(") || value.back() != ')') { continue; }
        if (value.find(')') != value.size() - 1) { continue; }

        std::string inside = value.substr(4, value.size() - 5);

        std::size_t first = 0;
        std::size_t last = inside.size();
        while (first < last && (inside[first] == ' ' || inside[first] == '"' || inside[first] == '\'')) { first++; }
        while (last > first && (inside[last - 1] == ' ' || inside[last - 1] == '"' || inside[last - 1] == '\''))
        {
          last--;
        }

        inside = inside.substr(first, last - first);
        if (inside.empty()) { continue; }

        value = "url(\"" + ResolveUiPath(sheet, inside) + "\")";
      }
    }
  }

  std::string ResolveUiPath(const std::string &base, const std::string &path)
  {
    if (path.find("://") != std::string::npos) { return path; }

    const std::size_t scheme_end = base.find("://");
    const std::string scheme = scheme_end == std::string::npos ? "" : base.substr(0, scheme_end + 3);
    const std::string rest = scheme_end == std::string::npos ? base : base.substr(scheme_end + 3);

    // the folders of what names the path, without its own name
    std::vector<std::string> folders;
    std::string folder;

    for (const char letter : rest)
    {
      if (letter == '/')
      {
        if (!folder.empty()) { folders.push_back(folder); }
        folder.clear();
      } else
      {
        folder += letter;
      }
    }

    // a path from the top of the scheme
    std::string relative = path;
    if (!relative.empty() && relative[0] == '/')
    {
      folders.clear();
      relative = relative.substr(1);
    }

    std::string part;
    const auto take = [&]
    {
      if (part == "..")
      {
        if (!folders.empty()) { folders.pop_back(); }
      } else if (!part.empty() && part != ".")
      {
        folders.push_back(part);
      }
      part.clear();
    };

    for (const char letter : relative)
    {
      if (letter == '/') { take(); }
      else { part += letter; }
    }
    take();

    std::string resolved = scheme;
    for (std::size_t i = 0; i < folders.size(); i++)
    {
      if (i > 0) { resolved += '/'; }
      resolved += folders[i];
    }

    return resolved;
  }

  void UiStyleSheets::Add(
    const CssStyleSheet &sheet,
    const std::vector<std::size_t> &media,
    const std::vector<std::size_t> &own_media,
    std::vector<std::string> &warnings)
  {
    // the conditions of the sheet, as places in the list of all of them
    const auto conditions_of = [&](const std::vector<std::size_t> &of_sheet)
    {
      std::vector<std::size_t> conditions = media;
      for (const std::size_t index : of_sheet)
      {
        if (index < own_media.size()) { conditions.push_back(own_media[index]); }
      }
      return conditions;
    };

    for (const auto &font : sheet.fonts)
    {
      _fonts.push_back({font.family, font.weight, ResolveUiPath(sheet.path, font.source)});
    }

    for (const auto &rule : sheet.rules)
    {
      const std::string where = "the rule '" + rule.selector_text + "'";

      std::vector<CssDeclaration> declarations = rule.declarations;
      Check(declarations, sheet.path, where, warnings);
      ResolveUrls(declarations, sheet.path);

      const auto shared = std::make_shared<const std::vector<CssDeclaration>>(std::move(declarations));

      for (const auto &selector : rule.selectors)
      {
        Rule added;
        added.selector = selector;
        added.declarations = shared;
        added.sheet = sheet.path;
        added.where = where;
        added.media = conditions_of(rule.media);
        added.order = _rules.size();

        _dependencies.Add(selector);
        _rules.push_back(std::move(added));
      }
    }

    for (const auto &keyframes : sheet.keyframes)
    {
      Keyframes added;
      added.name = keyframes.name;
      added.frames = keyframes.frames;
      added.sheet = sheet.path;
      added.media = conditions_of(keyframes.media);

      for (auto &frame : added.frames)
      {
        // a keyframe says how the way to the next one takes its time, and
        // nothing else about animations
        std::erase_if(frame.declarations, [&](const CssDeclaration &declaration)
        {
          const bool is_animation = declaration.name.starts_with("animation") &&
                                    declaration.name != "animation-timing-function";
          const bool is_transition = declaration.name.starts_with("transition");

          if (!is_animation && !is_transition) { return false; }

          warnings.push_back(WithLine(
            sheet.path, declaration.line,
            std::format(
              "'{}' of @keyframes {} cannot be animated. The declaration is left out",
              declaration.name, keyframes.name)));
          return true;
        });

        Check(frame.declarations, sheet.path, "@keyframes " + keyframes.name, warnings);
        ResolveUrls(frame.declarations, sheet.path);
      }

      _keyframes.push_back(std::move(added));
    }
  }

  bool UiStyleSheets::ReadSheet(
    const std::string &path,
    const std::vector<std::size_t> &media,
    FileSystemContext &file_system,
    std::vector<std::string> &errors,
    std::vector<std::string> &warnings,
    const std::string &named_by)
  {
    if (std::ranges::find(_reading, path) != _reading.end())
    {
      warnings.push_back(std::format(
        "{}imports {}, which imports it in turn. The sheet is left out", named_by, path));
      return true;
    }

    std::string text;
    if (!file_system.ReadText(path, text))
    {
      errors.push_back(std::format("{}the style sheet {} cannot be read", named_by, path));
      return false;
    }

    CssStyleSheet sheet;
    ParseCssStyleSheet(text, path, sheet, warnings);

    // the conditions of the sheet join those of all sheets
    std::vector<std::size_t> own_media;
    for (const auto &list : sheet.media)
    {
      own_media.push_back(_media.size());
      _media.push_back(list);
      _media_holds.push_back(list.Matches(_environment));
    }

    _reading.push_back(path);
    bool is_read = true;

    // what a sheet imports comes in front of what it holds itself
    for (const auto &import : sheet.imports)
    {
      std::vector<std::size_t> conditions = media;
      if (import.media >= 0 && static_cast<std::size_t>(import.media) < own_media.size())
      {
        conditions.push_back(own_media[static_cast<std::size_t>(import.media)]);
      }

      const std::string from = std::format("{}:{}: ", path, import.line);
      if (!ReadSheet(ResolveUiPath(path, import.path), conditions, file_system, errors, warnings, from))
      {
        is_read = false;
      }
    }

    _reading.pop_back();

    Add(sheet, media, own_media, warnings);
    return is_read;
  }

  bool UiStyleSheets::Load(
    const std::vector<std::string> &paths,
    const std::string &base,
    FileSystemContext &file_system,
    std::vector<std::string> &errors,
    std::vector<std::string> &warnings)
  {
    const CssEnvironment environment = _environment;
    *this = UiStyleSheets{};
    _environment = environment;

    bool is_read = true;

    for (const auto &path : paths)
    {
      const std::string resolved = ResolveUiPath(base, path);
      _paths.push_back(resolved);

      if (!ReadSheet(resolved, {}, file_system, errors, warnings, base.empty() ? "" : base + ": "))
      {
        is_read = false;
      }
    }

    return is_read;
  }

  void UiStyleSheets::AddText(const std::string &text, const std::string &path, std::vector<std::string> &warnings)
  {
    CssStyleSheet sheet;
    ParseCssStyleSheet(text, path, sheet, warnings);

    std::vector<std::size_t> own_media;
    for (const auto &list : sheet.media)
    {
      own_media.push_back(_media.size());
      _media.push_back(list);
      _media_holds.push_back(list.Matches(_environment));
    }

    for (const auto &import : sheet.imports)
    {
      warnings.push_back(WithLine(
        path, import.line,
        std::format("@import of '{}' is not followed from a sheet that is no file", import.path)));
    }

    Add(sheet, {}, own_media, warnings);
  }

  const std::vector<std::string> &UiStyleSheets::GetPaths() const
  {
    return _paths;
  }

  bool UiStyleSheets::IsEmpty() const
  {
    return _rules.empty() && _keyframes.empty() && _fonts.empty();
  }

  const std::vector<UiStyleSheets::Rule> &UiStyleSheets::GetRules() const
  {
    return _rules;
  }

  bool UiStyleSheets::Holds(const std::vector<std::size_t> &media) const
  {
    return std::ranges::all_of(media, [this](const std::size_t index)
    {
      return index < _media_holds.size() && _media_holds[index];
    });
  }

  const UiStyleSheets::Keyframes *UiStyleSheets::FindKeyframes(const std::string &name) const
  {
    for (std::size_t i = _keyframes.size(); i > 0; i--)
    {
      const Keyframes &keyframes = _keyframes[i - 1];
      if (keyframes.name == name && Holds(keyframes.media)) { return &keyframes; }
    }
    return nullptr;
  }

  const std::vector<UiStyleSheets::Font> &UiStyleSheets::GetFonts() const
  {
    return _fonts;
  }

  const CssDependencies &UiStyleSheets::GetDependencies() const
  {
    return _dependencies;
  }

  bool UiStyleSheets::SetEnvironment(const CssEnvironment &environment)
  {
    _environment = environment;

    bool changed = false;
    for (std::size_t i = 0; i < _media.size(); i++)
    {
      const bool holds = _media[i].Matches(environment);
      if (holds != _media_holds[i]) { changed = true; }
      _media_holds[i] = holds;
    }

    return changed;
  }

  const CssEnvironment &UiStyleSheets::GetEnvironment() const
  {
    return _environment;
  }
} // neon
