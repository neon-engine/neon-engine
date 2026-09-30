#include "css-style-sheet.hpp"

#include <algorithm>
#include <cctype>
#include <format>

#include <neon/ui/css-values.hpp>

namespace neon
{
  namespace
  {
    bool IsSpace(const char letter)
    {
      return letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r' || letter == '\f';
    }

    std::string Trimmed(const std::string &text)
    {
      std::size_t first = 0;
      std::size_t last = text.size();

      while (first < last && IsSpace(text[first])) { first++; }
      while (last > first && IsSpace(text[last - 1])) { last--; }

      return text.substr(first, last - first);
    }

    std::string Lowered(std::string text)
    {
      std::ranges::transform(text, text.begin(), [](const unsigned char letter)
      {
        return static_cast<char>(std::tolower(letter));
      });
      return text;
    }

    /// One line, for a message: line feeds and what follows them are left
    /// out, and a long text is cut off.
    std::string Shortened(const std::string &text)
    {
      std::string result;
      bool was_space = false;

      for (const char letter : Trimmed(text))
      {
        if (IsSpace(letter))
        {
          was_space = true;
          continue;
        }

        if (was_space) { result += ' '; }
        was_space = false;
        result += letter;
      }

      if (result.size() > 60) { result = result.substr(0, 57) + "..."; }
      return result;
    }

    bool IsName(const std::string &text)
    {
      if (text.empty()) { return false; }

      std::size_t start = 0;
      if (text.starts_with("--")) { return text.size() > 2; }
      if (text[0] == '-') { start = 1; }

      if (start >= text.size()) { return false; }

      const auto first = static_cast<unsigned char>(text[start]);
      if (std::isalpha(first) == 0 && first != '_' && first < 0x80) { return false; }

      return std::ranges::all_of(text, [](const char letter)
      {
        const auto byte = static_cast<unsigned char>(letter);
        return std::isalnum(byte) != 0 || letter == '-' || letter == '_' || byte >= 0x80;
      });
    }

    /// Without the quotes around it, when it has them.
    std::string Unquoted(const std::string &text)
    {
      const std::string trimmed = Trimmed(text);

      if (trimmed.size() >= 2 && (trimmed.front() == '"' || trimmed.front() == '\'') &&
          trimmed.back() == trimmed.front())
      {
        std::string inside;
        for (std::size_t i = 1; i + 1 < trimmed.size(); i++)
        {
          if (trimmed[i] == '\\' && i + 2 < trimmed.size()) { i++; }
          inside += trimmed[i];
        }
        return inside;
      }

      return trimmed;
    }

    /// What `url(...)` holds, or the text in quotes. Returns false when
    /// the text is neither. `rest` is what follows.
    bool ReadUrl(const std::string &text, std::string &url, std::string &rest)
    {
      const std::string trimmed = Trimmed(text);

      if (Lowered(trimmed.substr(0, 4)) == "url(")
      {
        const std::size_t close = trimmed.find(')');
        if (close == std::string::npos) { return false; }

        url = Unquoted(trimmed.substr(4, close - 4));
        rest = Trimmed(trimmed.substr(close + 1));
        return !url.empty();
      }

      if (!trimmed.empty() && (trimmed[0] == '"' || trimmed[0] == '\''))
      {
        std::size_t close = 1;
        while (close < trimmed.size() && trimmed[close] != trimmed[0])
        {
          if (trimmed[close] == '\\') { close++; }
          close++;
        }

        if (close >= trimmed.size()) { return false; }

        url = Unquoted(trimmed.substr(0, close + 1));
        rest = Trimmed(trimmed.substr(close + 1));
        return !url.empty();
      }

      return false;
    }

    class Parser
    {
      const std::string &_text;
      std::string _path;
      std::vector<std::string> *_problems;

      std::size_t _position = 0;
      std::size_t _line = 1;

      void Report(const std::size_t line, const std::string &message) const
      {
        _problems->push_back(std::format("{}:{}: {}", _path, line, message));
      }

      [[nodiscard]] bool IsAtEnd() const
      {
        return _position >= _text.size();
      }

      [[nodiscard]] char Peek(const std::size_t ahead = 0) const
      {
        return _position + ahead < _text.size() ? _text[_position + ahead] : '\0';
      }

      char Step()
      {
        const char letter = _text[_position];
        if (letter == '\n') { _line++; }
        _position++;
        return letter;
      }

      /// Returns false when the comment is not closed, which ends it
      /// where the text ends.
      bool SkipComment()
      {
        const std::size_t start = _line;
        Step();
        Step();

        while (!IsAtEnd())
        {
          if (Peek() == '*' && Peek(1) == '/')
          {
            Step();
            Step();
            return true;
          }
          Step();
        }

        Report(start, "a comment is not closed, where */ was expected");
        return false;
      }

      void SkipSpacesAndComments()
      {
        while (!IsAtEnd())
        {
          if (IsSpace(Peek()))
          {
            Step();
          } else if (Peek() == '/' && Peek(1) == '*')
          {
            SkipComment();
          } else if (Peek() == '<' && _text.compare(_position, 4, "<!--") == 0)
          {
            // what hid a style sheet from the browsers of the past
            for (int i = 0; i < 4; i++) { Step(); }
          } else if (Peek() == '-' && _text.compare(_position, 3, "-->") == 0)
          {
            for (int i = 0; i < 3; i++) { Step(); }
          } else
          {
            break;
          }
        }
      }

      /// A text in quotes, as it is written, quotes included.
      void ReadQuoted(std::string &into)
      {
        const std::size_t start = _line;
        const char quote = Step();
        into += quote;

        while (!IsAtEnd())
        {
          const char letter = Peek();

          if (letter == '\\' && _position + 1 < _text.size())
          {
            into += Step();
            into += Step();
            continue;
          }

          // a text in quotes ends with its line
          if (letter == '\n') { break; }

          into += Step();
          if (letter == quote) { return; }
        }

        Report(start, "a text in quotes is not closed");
        into += quote;
      }

      /// Reads up to one of the characters in `stops` that is outside of
      /// brackets and quotes. Comments become a space. The reader stands
      /// on the character it stopped at.
      std::string ReadUntil(const std::string &stops)
      {
        std::string read;
        int depth = 0;

        while (!IsAtEnd())
        {
          const char letter = Peek();

          if (letter == '/' && Peek(1) == '*')
          {
            SkipComment();
            read += ' ';
            continue;
          }

          if (letter == '"' || letter == '\'')
          {
            ReadQuoted(read);
            continue;
          }

          if (depth == 0 && stops.find(letter) != std::string::npos) { break; }

          if (letter == '(' || letter == '[') { depth++; }
          if ((letter == ')' || letter == ']') && depth > 0) { depth--; }

          read += Step();
        }

        return read;
      }

      /// Skips a block with everything inside it. The reader stands on
      /// its opening bracket.
      void SkipBlock()
      {
        const std::size_t start = _line;
        int depth = 0;

        while (!IsAtEnd())
        {
          const char letter = Peek();

          if (letter == '/' && Peek(1) == '*')
          {
            SkipComment();
            continue;
          }

          if (letter == '"' || letter == '\'')
          {
            std::string ignored;
            ReadQuoted(ignored);
            continue;
          }

          Step();

          if (letter == '{') { depth++; }
          if (letter == '}')
          {
            depth--;
            if (depth == 0) { return; }
          }
        }

        Report(start, "a block is not closed, where } was expected");
      }

      /// Expects the reader on an opening bracket, and steps over it.
      void ReadDeclarationBlock(std::vector<CssDeclaration> &declarations, const std::string &of)
      {
        const std::size_t start = _line;
        Step();

        ReadDeclarations(declarations, of, true);

        if (IsAtEnd())
        {
          Report(start, std::format("the block of {} is not closed, where }} was expected", of));
          return;
        }

        Step();
      }

    public:
      Parser(const std::string &text, const std::string &path, std::vector<std::string> *problems)
        : _text(text)
      {
        _path = path;
        _problems = problems;
      }

      void SetLine(const std::size_t line)
      {
        _line = line;
      }

      /// Reads declarations up to the closing bracket of their block, or
      /// to the end of the text.
      void ReadDeclarations(std::vector<CssDeclaration> &declarations, const std::string &of, const bool in_block)
      {
        while (true)
        {
          SkipSpacesAndComments();

          if (IsAtEnd()) { return; }

          if (Peek() == '}')
          {
            if (in_block) { return; }

            Report(_line, "'}' closes a block that was not opened");
            Step();
            continue;
          }

          if (Peek() == ';')
          {
            Step();
            continue;
          }

          const std::size_t line = _line;
          const std::string front = ReadUntil(":;{}");

          if (IsAtEnd() || Peek() != ':')
          {
            if (!IsAtEnd() && Peek() == '{')
            {
              Report(line, std::format(
                       "'{}' of {} starts a block, where a declaration such as color: red was expected. "
                       "Rules inside of rules are not read",
                       Shortened(front), of));
              SkipBlock();
              continue;
            }

            Report(line, std::format(
                     "'{}' of {} has no colon, where a declaration such as color: red was expected",
                     Shortened(front), of));
            continue;
          }

          Step();

          std::string value = ReadUntil(";}");

          CssDeclaration declaration;
          declaration.line = line;
          declaration.name = Trimmed(front);

          if (!IsName(declaration.name))
          {
            Report(line, std::format(
                     "'{}' of {} is not the name of a property, which is made of letters, digits, and "
                     "hyphens",
                     Shortened(declaration.name), of));
            continue;
          }

          if (!declaration.IsCustomProperty()) { declaration.name = Lowered(declaration.name); }

          value = Trimmed(value);

          // `!important`, with spaces where they are allowed
          if (const std::size_t mark = value.rfind('!'); mark != std::string::npos)
          {
            const std::string behind = Lowered(Trimmed(value.substr(mark + 1)));

            if (behind == "important")
            {
              declaration.is_important = true;
              value = Trimmed(value.substr(0, mark));
            } else if (value.find('"') == std::string::npos && value.find('\'') == std::string::npos)
            {
              Report(line, std::format(
                       "'{}' of {} has '!{}' behind its value, where !important or nothing was expected",
                       declaration.name, of, Shortened(behind)));
              continue;
            }
          }

          if (value.empty() && !declaration.IsCustomProperty())
          {
            Report(line, std::format(
                     "'{}' of {} has no value, where one was expected behind the colon",
                     declaration.name, of));
            continue;
          }

          declaration.value = value;
          declarations.push_back(declaration);
        }
      }

      void ReadFontFace(CssStyleSheet &sheet, const std::size_t line)
      {
        std::vector<CssDeclaration> declarations;
        ReadDeclarationBlock(declarations, "@font-face");

        CssFontFace face;
        face.line = line;
        bool is_wrong = false;

        for (const auto &declaration : declarations)
        {
          if (declaration.name == "font-family")
          {
            face.family = Unquoted(declaration.value);
          } else if (declaration.name == "src")
          {
            // the first of the sources, since there is one kind of font
            std::string rest;
            if (!ReadUrl(declaration.value, face.source, rest))
            {
              Report(declaration.line, std::format(
                       "'src' of @font-face is '{}', where url() with the path of a font was expected, "
                       "such as url(\"fonts/title.ttf\")",
                       Shortened(declaration.value)));
              is_wrong = true;
            }
          } else if (declaration.name == "font-weight")
          {
            const std::string value = Lowered(declaration.value);
            float weight = 0.0f;

            if (value == "normal")
            {
              face.weight = 400;
            } else if (value == "bold")
            {
              face.weight = 700;
            } else if (ParseCssNumber(value, weight) && weight >= 1.0f && weight <= 1000.0f)
            {
              face.weight = static_cast<int>(weight + 0.5f);
            } else
            {
              Report(declaration.line, std::format(
                       "'font-weight' of @font-face is '{}', where a number from 1 to 1000, normal, or "
                       "bold was expected",
                       Shortened(declaration.value)));
              is_wrong = true;
            }
          } else if (declaration.name != "font-style" && declaration.name != "font-display" &&
                     declaration.name != "font-stretch" && declaration.name != "unicode-range")
          {
            Report(declaration.line, std::format(
                     "'{}' is not known to @font-face. Known are: font-family, src, font-weight",
                     declaration.name));
          }
        }

        if (face.family.empty())
        {
          Report(line, "@font-face has no 'font-family', where the name it is asked for by was expected");
          is_wrong = true;
        }

        if (face.source.empty() && !is_wrong)
        {
          Report(line, "@font-face has no 'src', where url() with the path of a font was expected");
          is_wrong = true;
        }

        if (!is_wrong) { sheet.fonts.push_back(face); }
      }

      void ReadKeyframes(
        CssStyleSheet &sheet,
        const std::string &prelude,
        const std::size_t line,
        const std::vector<std::size_t> &media)
      {
        CssKeyframes keyframes;
        keyframes.name = Unquoted(prelude);
        keyframes.line = line;
        keyframes.media = media;

        const bool has_name = !keyframes.name.empty() && (IsName(keyframes.name) || Trimmed(prelude)[0] == '"' ||
                                                           Trimmed(prelude)[0] == '\'');
        if (!has_name)
        {
          Report(line, std::format(
                   "@keyframes is followed by '{}', where the name of the animation was expected",
                   Shortened(prelude)));
        }

        const std::size_t start = _line;
        Step();

        while (true)
        {
          SkipSpacesAndComments();

          if (IsAtEnd())
          {
            Report(start, std::format(
                     "the block of @keyframes {} is not closed, where }} was expected", keyframes.name));
            break;
          }

          if (Peek() == '}')
          {
            Step();
            break;
          }

          const std::size_t frame_line = _line;
          const std::string selector = ReadUntil("{};");

          if (IsAtEnd() || Peek() != '{')
          {
            Report(frame_line, std::format(
                     "'{}' of @keyframes {} has no block, where from, to, or a percentage and a block "
                     "were expected",
                     Shortened(selector), keyframes.name));

            if (!IsAtEnd() && Peek() == ';') { Step(); }
            continue;
          }

          // `from`, `to`, and percentages, set apart by commas
          std::vector<float> offsets;
          bool is_wrong = false;

          std::size_t part_start = 0;
          const std::string all = selector + ",";

          for (std::size_t i = 0; i < all.size(); i++)
          {
            if (all[i] != ',') { continue; }

            const std::string part = Lowered(Trimmed(all.substr(part_start, i - part_start)));
            part_start = i + 1;

            float percent = 0.0f;

            if (part == "from")
            {
              offsets.push_back(0.0f);
            } else if (part == "to")
            {
              offsets.push_back(1.0f);
            } else if (part.size() > 1 && part.back() == '%' &&
                       ParseCssNumber(part.substr(0, part.size() - 1), percent) && percent >= 0.0f &&
                       percent <= 100.0f)
            {
              offsets.push_back(percent / 100.0f);
            } else
            {
              Report(frame_line, std::format(
                       "'{}' of @keyframes {} is no part of the way, where from, to, or a percentage "
                       "from 0% to 100% was expected",
                       Shortened(part), keyframes.name));
              is_wrong = true;
            }
          }

          std::vector<CssDeclaration> declarations;
          ReadDeclarationBlock(declarations, std::format("@keyframes {}", keyframes.name));

          if (is_wrong) { continue; }

          std::erase_if(declarations, [&](const CssDeclaration &declaration)
          {
            if (!declaration.is_important) { return false; }

            Report(declaration.line, std::format(
                     "'{}' of @keyframes {} is !important, which a keyframe cannot be. It is left out",
                     declaration.name, keyframes.name));
            return true;
          });

          for (const float offset : offsets) { keyframes.frames.push_back({offset, declarations, frame_line}); }
        }

        if (!has_name) { return; }

        std::ranges::stable_sort(keyframes.frames, [](const CssKeyframe &a, const CssKeyframe &b)
        {
          return a.offset < b.offset;
        });

        sheet.keyframes.push_back(std::move(keyframes));
      }

      void ReadImport(
        CssStyleSheet &sheet,
        const std::string &prelude,
        const std::size_t line,
        const bool comes_first)
      {
        CssImport import;
        import.line = line;

        std::string rest;
        if (!ReadUrl(prelude, import.path, rest))
        {
          Report(line, std::format(
                   "@import is followed by '{}', where the path of a style sheet was expected, such as "
                   "\"theme.css\" or url(\"theme.css\")",
                   Shortened(prelude)));
          return;
        }

        if (!comes_first)
        {
          Report(line, std::format(
                   "@import of '{}' comes behind a rule and is left out. It has to come first, as CSS says",
                   import.path));
          return;
        }

        if (!rest.empty())
        {
          CssMediaQueryList media;
          if (std::string error; !ParseCssMedia(rest, media, error))
          {
            Report(line, std::format(
                     "the condition '{}' of @import cannot be read: {}. The sheet is left out",
                     Shortened(rest), error));
            return;
          }

          import.media = static_cast<int>(sheet.media.size());
          sheet.media.push_back(media);
        }

        sheet.imports.push_back(import);
      }

      /// Reads rules up to the closing bracket of the block they are in,
      /// or to the end of the text.
      void ReadRules(CssStyleSheet &sheet, const std::vector<std::size_t> &media, const bool in_block)
      {
        // whether nothing but @import and @charset came so far
        bool comes_first = !in_block && sheet.rules.empty() && sheet.keyframes.empty() && sheet.fonts.empty();

        while (true)
        {
          SkipSpacesAndComments();

          if (IsAtEnd()) { return; }

          if (Peek() == '}')
          {
            if (in_block) { return; }

            Report(_line, "'}' closes a block that was not opened");
            Step();
            continue;
          }

          if (Peek() == ';')
          {
            Step();
            continue;
          }

          const std::size_t line = _line;

          if (Peek() == '@')
          {
            Step();

            std::string name;
            while (!IsAtEnd() && (std::isalnum(static_cast<unsigned char>(Peek())) != 0 || Peek() == '-'))
            {
              name += Step();
            }
            name = Lowered(name);

            const std::string prelude = Trimmed(ReadUntil("{;}"));
            const bool has_block = !IsAtEnd() && Peek() == '{';

            if (name == "import")
            {
              if (has_block)
              {
                Report(line, "@import is followed by a block, where a path and a semicolon were expected");
                SkipBlock();
                continue;
              }

              if (!IsAtEnd() && Peek() == ';') { Step(); }
              ReadImport(sheet, prelude, line, comes_first);
              continue;
            }

            if (name == "charset")
            {
              // text is UTF-8, whatever this says
              if (has_block) { SkipBlock(); }
              else if (!IsAtEnd() && Peek() == ';') { Step(); }
              continue;
            }

            comes_first = false;

            if (!has_block)
            {
              Report(line, std::format(
                       "@{} has no block, where {{ was expected", name.empty() ? "" : name));

              if (!IsAtEnd() && Peek() == ';') { Step(); }
              continue;
            }

            if (name == "media")
            {
              CssMediaQueryList list;
              std::string error;

              if (prelude.empty())
              {
                Report(line, "@media has no condition, where one such as (min-width: 800px) was expected");
                SkipBlock();
                continue;
              }

              if (!ParseCssMedia(prelude, list, error))
              {
                Report(line, std::format(
                         "the condition '{}' of @media cannot be read: {}. What it holds is left out",
                         Shortened(prelude), error));
                SkipBlock();
                continue;
              }

              std::vector<std::size_t> inner = media;
              inner.push_back(sheet.media.size());
              sheet.media.push_back(list);

              const std::size_t start = _line;
              Step();
              ReadRules(sheet, inner, true);

              if (IsAtEnd())
              {
                Report(start, "the block of @media is not closed, where } was expected");
              } else
              {
                Step();
              }
            } else if (name == "font-face")
            {
              if (!prelude.empty())
              {
                Report(line, std::format(
                         "@font-face is followed by '{}', where {{ was expected", Shortened(prelude)));
              }
              ReadFontFace(sheet, line);
            } else if (name == "keyframes" || name == "-webkit-keyframes")
            {
              ReadKeyframes(sheet, prelude, line, media);
            } else
            {
              Report(line, std::format(
                       "@{} is not known, and what it holds is left out. Known are: @import, @media, "
                       "@font-face, @keyframes",
                       name));
              SkipBlock();
            }

            continue;
          }

          comes_first = false;

          const std::string prelude = ReadUntil("{;}");

          if (IsAtEnd() || Peek() != '{')
          {
            Report(line, std::format(
                     "'{}' is followed by no block, where a rule such as button {{ color: red; }} was "
                     "expected",
                     Shortened(prelude)));

            if (!IsAtEnd() && Peek() == ';') { Step(); }
            continue;
          }

          CssRule rule;
          rule.line = line;
          rule.media = media;
          rule.selector_text = Shortened(prelude);

          std::string error;
          const bool is_read = ParseCssSelectors(prelude, rule.selectors, error);

          if (!is_read)
          {
            Report(line, std::format(
                     "the selector '{}' cannot be read: {}. The rule is left out",
                     rule.selector_text, error));
          }

          ReadDeclarationBlock(rule.declarations, std::format("the rule '{}'", rule.selector_text));

          if (is_read) { sheet.rules.push_back(std::move(rule)); }
        }
      }
    };
  }

  void ParseCssStyleSheet(
    const std::string &text,
    const std::string &path,
    CssStyleSheet &sheet,
    std::vector<std::string> &problems)
  {
    sheet = CssStyleSheet{};
    sheet.path = path;

    // a mark of the order of bytes is no part of the text
    std::size_t start = 0;
    if (text.size() >= 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0) { start = 3; }

    const std::string content = start > 0 ? text.substr(start) : text;

    Parser parser(content, path, &problems);
    parser.ReadRules(sheet, {}, false);
  }

  void ParseCssDeclarations(
    const std::string &text,
    const std::string &path,
    const std::size_t first_line,
    std::vector<CssDeclaration> &declarations,
    std::vector<std::string> &problems)
  {
    Parser parser(text, path, &problems);
    parser.SetLine(first_line);
    parser.ReadDeclarations(declarations, "the style", false);
  }
} // neon
