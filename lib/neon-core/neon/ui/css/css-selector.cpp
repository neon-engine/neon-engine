#include "css-selector.hpp"

#include <algorithm>
#include <cctype>
#include <format>

namespace neon
{
  namespace
  {
    const std::vector<std::string> state_names = {
      "hover", "active", "focus", "focus-within", "disabled", "enabled", "checked", "valid", "invalid"
    };

    const std::vector<std::string> structural_names = {
      "first-child", "last-child", "only-child", "nth-child", "nth-last-child", "empty"
    };

    bool IsSpace(const char letter)
    {
      return letter == ' ' || letter == '\t' || letter == '\n' || letter == '\r' || letter == '\f';
    }

    bool IsNameStart(const char letter)
    {
      return std::isalpha(static_cast<unsigned char>(letter)) != 0 || letter == '_' ||
             static_cast<unsigned char>(letter) >= 0x80;
    }

    bool IsNamePart(const char letter)
    {
      return IsNameStart(letter) || (letter >= '0' && letter <= '9') || letter == '-';
    }

    std::string Lowered(std::string text)
    {
      std::ranges::transform(text, text.begin(), [](const unsigned char letter)
      {
        return static_cast<char>(std::tolower(letter));
      });
      return text;
    }

    bool Contains(const std::vector<std::string> &names, const std::string &name)
    {
      return std::ranges::find(names, name) != names.end();
    }

    /// Reads selectors from a text, from the left to the right.
    class Reader
    {
      const std::string &_text;
      std::size_t _position = 0;
      std::string _error;

    public:
      explicit Reader(const std::string &text) : _text(text) {}

      [[nodiscard]] const std::string &GetError() const
      {
        return _error;
      }

      [[nodiscard]] bool IsAtEnd() const
      {
        return _position >= _text.size();
      }

      [[nodiscard]] char Peek(const std::size_t ahead = 0) const
      {
        return _position + ahead < _text.size() ? _text[_position + ahead] : '\0';
      }

      bool Fail(const std::string &error)
      {
        if (_error.empty()) { _error = error; }
        return false;
      }

      bool SkipSpaces()
      {
        const std::size_t before = _position;
        while (!IsAtEnd() && IsSpace(Peek())) { _position++; }
        return _position > before;
      }

      /// A name as CSS writes one: letters, digits, hyphens, and
      /// underscores, not starting with a digit. A backslash takes the
      /// character behind it as it is.
      bool ReadName(std::string &name)
      {
        name.clear();
        const std::size_t start = _position;

        if (Peek() == '-')
        {
          name += '-';
          _position++;

          // two hyphens start the name of a custom property
          if (Peek() == '-')
          {
            name += '-';
            _position++;
          }
        }

        if (!IsNameStart(Peek()) && Peek() != '\\' && name != "--")
        {
          _position = start;
          name.clear();
          return false;
        }

        while (!IsAtEnd())
        {
          if (Peek() == '\\' && _position + 1 < _text.size())
          {
            name += _text[_position + 1];
            _position += 2;
          } else if (IsNamePart(Peek()))
          {
            name += Peek();
            _position++;
          } else
          {
            break;
          }
        }

        if (name.empty() || name == "-")
        {
          _position = start;
          name.clear();
          return false;
        }
        return true;
      }

      bool ReadText(std::string &text)
      {
        const char quote = Peek();
        if (quote != '"' && quote != '\'') { return false; }

        _position++;
        text.clear();

        while (!IsAtEnd() && Peek() != quote)
        {
          if (Peek() == '\\' && _position + 1 < _text.size()) { _position++; }
          text += Peek();
          _position++;
        }

        if (IsAtEnd()) { return Fail("a text in quotes is not closed"); }

        _position++;
        return true;
      }

      /// What is between brackets, which may hold brackets of its own.
      /// The reader stands behind the opening bracket.
      bool ReadInside(std::string &inside)
      {
        inside.clear();
        int depth = 1;

        while (!IsAtEnd())
        {
          const char letter = Peek();

          if (letter == '"' || letter == '\'')
          {
            const std::size_t start = _position;
            if (std::string ignored; !ReadText(ignored)) { return false; }
            inside += _text.substr(start, _position - start);
            continue;
          }

          if (letter == '(') { depth++; }
          if (letter == ')')
          {
            depth--;
            if (depth == 0)
            {
              _position++;
              return true;
            }
          }

          inside += letter;
          _position++;
        }

        return false;
      }

      void Step()
      {
        _position++;
      }

      [[nodiscard]] std::size_t GetPosition() const
      {
        return _position;
      }

      /// What is left from here on.
      [[nodiscard]] std::string Rest() const
      {
        return _position < _text.size() ? _text.substr(_position) : "";
      }
    };

    std::string Trimmed(const std::string &text)
    {
      std::size_t first = 0;
      std::size_t last = text.size();

      while (first < last && IsSpace(text[first])) { first++; }
      while (last > first && IsSpace(text[last - 1])) { last--; }

      return text.substr(first, last - first);
    }

    /// `an+b` of `:nth-child()`, with `odd` and `even`.
    bool ParseNth(const std::string &written, int &step, int &offset)
    {
      std::string text;
      for (const char letter : Lowered(written))
      {
        if (!IsSpace(letter)) { text += letter; }
      }

      if (text == "odd")
      {
        step = 2;
        offset = 1;
        return true;
      }

      if (text == "even")
      {
        step = 2;
        offset = 0;
        return true;
      }

      if (text.empty()) { return false; }

      const auto read_whole = [](const std::string &digits, int &number)
      {
        if (digits.empty() || digits.size() > 9) { return false; }

        number = 0;
        for (const char digit : digits)
        {
          if (digit < '0' || digit > '9') { return false; }
          number = number * 10 + (digit - '0');
        }
        return true;
      };

      const std::size_t n = text.find('n');

      if (n == std::string::npos)
      {
        // a number alone
        std::size_t start = 0;
        int sign = 1;
        if (text[0] == '+' || text[0] == '-')
        {
          sign = text[0] == '-' ? -1 : 1;
          start = 1;
        }

        int number = 0;
        if (!read_whole(text.substr(start), number)) { return false; }

        step = 0;
        offset = sign * number;
        return true;
      }

      // what is in front of the n
      const std::string front = text.substr(0, n);
      if (front.empty() || front == "+")
      {
        step = 1;
      } else if (front == "-")
      {
        step = -1;
      } else
      {
        std::size_t start = 0;
        int sign = 1;
        if (front[0] == '+' || front[0] == '-')
        {
          sign = front[0] == '-' ? -1 : 1;
          start = 1;
        }

        int number = 0;
        if (!read_whole(front.substr(start), number)) { return false; }
        step = sign * number;
      }

      // and what is behind it
      const std::string back = text.substr(n + 1);
      if (back.empty())
      {
        offset = 0;
        return true;
      }

      if (back[0] != '+' && back[0] != '-') { return false; }

      int number = 0;
      if (!read_whole(back.substr(1), number)) { return false; }

      offset = back[0] == '-' ? -number : number;
      return true;
    }

    bool ParseComplex(Reader &reader, CssComplexSelector &selector, bool allows_pseudo_element);

    bool ParseList(
      const std::string &text,
      std::vector<CssComplexSelector> &selectors,
      std::string &error,
      bool allows_pseudo_element);

    bool ParsePseudoClass(Reader &reader, CssSimpleSelector &simple, CssSpecificity &specificity)
    {
      std::string name;
      if (!reader.ReadName(name)) { return reader.Fail("':' is followed by no name of a pseudo-class"); }

      name = Lowered(name);
      simple.kind = CssSimpleSelector::Kind::PseudoClass;
      simple.name = name;

      const bool has_arguments = reader.Peek() == '(';
      std::string inside;

      if (has_arguments)
      {
        reader.Step();
        if (!reader.ReadInside(inside)) { return reader.Fail(std::format("':{}(' is not closed", name)); }
      }

      if (name == "focus-visible")
      {
        // the focus is always shown, since it is moved with keys and a
        // controller
        simple.name = "focus";
      }

      if (name == "not" || name == "is" || name == "where")
      {
        if (!has_arguments) { return reader.Fail(std::format("':{}' has no selector in brackets", name)); }

        std::vector<CssComplexSelector> arguments;
        if (std::string error; !ParseList(inside, arguments, error, false))
        {
          return reader.Fail(std::format("':{}({})' cannot be read: {}", name, Trimmed(inside), error));
        }

        CssSpecificity most;
        for (auto &argument : arguments)
        {
          if (most < argument.specificity) { most = argument.specificity; }
          simple.arguments.push_back(std::make_shared<CssComplexSelector>(std::move(argument)));
        }

        // `:where()` counts for nothing, the others for the most their
        // arguments count for
        if (name != "where")
        {
          specificity.ids += most.ids;
          specificity.classes += most.classes;
          specificity.types += most.types;
        }
        return true;
      }

      if (name == "nth-child" || name == "nth-last-child")
      {
        if (!has_arguments || !ParseNth(inside, simple.step, simple.offset))
        {
          return reader.Fail(std::format(
            "':{}({})' cannot be read, where an+b was expected, such as 2n+1, odd, or 3",
            name, Trimmed(inside)));
        }

        specificity.classes++;
        return true;
      }

      if (has_arguments) { return reader.Fail(std::format("':{}' takes nothing in brackets", name)); }

      if (Contains(state_names, simple.name) || Contains(structural_names, name) || name == "root" ||
          name == "scope")
      {
        specificity.classes++;
        return true;
      }

      return reader.Fail(std::format(
        "':{}' is not a pseudo-class that is known. Known are: hover, active, focus, focus-visible, "
        "focus-within, disabled, enabled, checked, valid, invalid, first-child, last-child, only-child, "
        "nth-child(), nth-last-child(), empty, root, scope, not(), is(), where()",
        name));
    }

    bool ParseAttribute(Reader &reader, CssSimpleSelector &simple)
    {
      simple.kind = CssSimpleSelector::Kind::Attribute;

      reader.SkipSpaces();
      if (!reader.ReadName(simple.name)) { return reader.Fail("'[' is followed by no name of an attribute"); }

      // in files the names have an underscore where CSS has a hyphen
      std::ranges::replace(simple.name, '-', '_');

      reader.SkipSpaces();

      if (reader.Peek() == ']')
      {
        reader.Step();
        simple.compare = CssSimpleSelector::Compare::Exists;
        return true;
      }

      if (reader.IsAtEnd()) { return reader.Fail(std::format("'[{}' is not closed", simple.name)); }

      switch (reader.Peek())
      {
        case '=': simple.compare = CssSimpleSelector::Compare::Equals;
          break;
        case '~': simple.compare = CssSimpleSelector::Compare::Word;
          break;
        case '|': simple.compare = CssSimpleSelector::Compare::Dash;
          break;
        case '^': simple.compare = CssSimpleSelector::Compare::Prefix;
          break;
        case '$': simple.compare = CssSimpleSelector::Compare::Suffix;
          break;
        case '*': simple.compare = CssSimpleSelector::Compare::Contains;
          break;
        default:
          return reader.Fail(std::format(
            "'[{}' is followed by '{}', where ], =, ~=, |=, ^=, $=, or *= was expected",
            simple.name, reader.Peek()));
      }

      if (simple.compare != CssSimpleSelector::Compare::Equals)
      {
        reader.Step();
        if (reader.Peek() != '=')
        {
          return reader.Fail(std::format("'[{}' has an operator without its '='", simple.name));
        }
      }
      reader.Step();
      reader.SkipSpaces();

      if (reader.Peek() == '"' || reader.Peek() == '\'')
      {
        if (!reader.ReadText(simple.value)) { return false; }
      } else
      {
        // a value without quotes is a name or a number
        while (!reader.IsAtEnd() && !IsSpace(reader.Peek()) && reader.Peek() != ']')
        {
          simple.value += reader.Peek();
          reader.Step();
        }

        if (simple.value.empty())
        {
          return reader.Fail(std::format("'[{}' compares with no value", simple.name));
        }
      }

      reader.SkipSpaces();

      if (reader.Peek() == 'i' || reader.Peek() == 'I')
      {
        simple.ignores_case = true;
        reader.Step();
        reader.SkipSpaces();
      } else if (reader.Peek() == 's' || reader.Peek() == 'S')
      {
        reader.Step();
        reader.SkipSpaces();
      }

      if (reader.Peek() != ']') { return reader.Fail(std::format("'[{}' is not closed", simple.name)); }

      reader.Step();
      return true;
    }

    /// Returns false without an error when there is no compound selector
    /// here.
    bool ParseCompound(
      Reader &reader,
      CssCompoundSelector &compound,
      CssSpecificity &specificity,
      std::string &pseudo_element,
      bool &found)
    {
      found = false;

      if (reader.Peek() == '*')
      {
        reader.Step();
        found = true;
      } else if (std::string type; reader.ReadName(type))
      {
        compound.type = type;
        specificity.types++;
        found = true;
      }

      while (!reader.IsAtEnd())
      {
        const char letter = reader.Peek();

        if (!pseudo_element.empty() && (letter == '.' || letter == '#' || letter == '[' || letter == ':'))
        {
          return reader.Fail(std::format(
            "'::{}' is followed by '{}', where the end of the selector was expected",
            pseudo_element, letter));
        }

        if (letter == '.')
        {
          reader.Step();

          CssSimpleSelector simple;
          simple.kind = CssSimpleSelector::Kind::Class;
          if (!reader.ReadName(simple.name)) { return reader.Fail("'.' is followed by no name of a class"); }

          compound.parts.push_back(simple);
          specificity.classes++;
        } else if (letter == '#')
        {
          reader.Step();

          CssSimpleSelector simple;
          simple.kind = CssSimpleSelector::Kind::Id;

          // the name of an element may start with a digit
          while (!reader.IsAtEnd() && IsNamePart(reader.Peek()))
          {
            simple.name += reader.Peek();
            reader.Step();
          }

          if (simple.name.empty()) { return reader.Fail("'#' is followed by no name of an element"); }

          compound.parts.push_back(simple);
          specificity.ids++;
        } else if (letter == '[')
        {
          reader.Step();

          CssSimpleSelector simple;
          if (!ParseAttribute(reader, simple)) { return false; }

          compound.parts.push_back(simple);
          specificity.classes++;
        } else if (letter == ':')
        {
          reader.Step();

          if (reader.Peek() == ':')
          {
            reader.Step();

            if (!reader.ReadName(pseudo_element))
            {
              return reader.Fail("'::' is followed by no name of a pseudo-element");
            }

            pseudo_element = Lowered(pseudo_element);

            // the names browsers took for the parts of a scrollbar
            if (pseudo_element.starts_with("-webkit-")) { pseudo_element = pseudo_element.substr(8); }

            specificity.types++;
          } else
          {
            CssSimpleSelector simple;
            if (!ParsePseudoClass(reader, simple, specificity)) { return false; }
            compound.parts.push_back(simple);
          }
        } else
        {
          break;
        }

        found = true;
      }

      return true;
    }

    bool ParseComplex(Reader &reader, CssComplexSelector &selector, const bool allows_pseudo_element)
    {
      reader.SkipSpaces();

      CssCombinator combinator = CssCombinator::None;

      while (true)
      {
        CssCompoundSelector compound;
        compound.combinator = combinator;

        if (!selector.pseudo_element.empty())
        {
          return reader.Fail(std::format(
            "'::{}' is followed by more, where the end of the selector was expected",
            selector.pseudo_element));
        }

        bool found = false;
        if (!ParseCompound(reader, compound, selector.specificity, selector.pseudo_element, found))
        {
          return false;
        }

        if (!found)
        {
          if (reader.IsAtEnd() || reader.Peek() == ',')
          {
            return reader.Fail(
              selector.compounds.empty() && combinator == CssCombinator::None
                ? "a selector is empty"
                : "a selector ends with a combinator, where a selector was expected behind it");
          }

          return reader.Fail(std::format("'{}' cannot start a selector", reader.Peek()));
        }

        if (!selector.pseudo_element.empty() && !allows_pseudo_element)
        {
          return reader.Fail(std::format("'::{}' cannot be written in brackets", selector.pseudo_element));
        }

        selector.compounds.push_back(compound);

        const bool had_space = reader.SkipSpaces();
        if (reader.IsAtEnd() || reader.Peek() == ',') { break; }

        switch (reader.Peek())
        {
          case '>': combinator = CssCombinator::Child;
            break;
          case '+': combinator = CssCombinator::Adjacent;
            break;
          case '~': combinator = CssCombinator::Sibling;
            break;
          default: combinator = CssCombinator::Descendant;
            break;
        }

        if (combinator != CssCombinator::Descendant)
        {
          reader.Step();
          reader.SkipSpaces();
        } else if (!had_space)
        {
          return reader.Fail(std::format("'{}' cannot be part of a selector", reader.Peek()));
        }
      }

      return true;
    }

    std::string Tidied(const std::string &text)
    {
      std::string tidy;
      bool was_space = false;

      for (const char letter : Trimmed(text))
      {
        if (IsSpace(letter))
        {
          was_space = true;
          continue;
        }

        if (was_space) { tidy += ' '; }
        was_space = false;
        tidy += letter;
      }

      return tidy;
    }

    bool ParseList(
      const std::string &text,
      std::vector<CssComplexSelector> &selectors,
      std::string &error,
      const bool allows_pseudo_element)
    {
      selectors.clear();

      if (Trimmed(text).empty())
      {
        error = "there is no selector";
        return false;
      }

      Reader reader(text);

      while (true)
      {
        reader.SkipSpaces();
        const std::size_t start = reader.GetPosition();

        CssComplexSelector selector;
        if (!ParseComplex(reader, selector, allows_pseudo_element))
        {
          error = reader.GetError();
          selectors.clear();
          return false;
        }

        selector.text = Tidied(text.substr(start, reader.GetPosition() - start));
        selectors.push_back(std::move(selector));

        reader.SkipSpaces();
        if (reader.IsAtEnd()) { break; }

        // a comma, since nothing else ends a selector
        reader.Step();
      }

      return true;
    }

    bool EqualsText(const std::string &a, const std::string &b, const bool ignores_case)
    {
      return ignores_case ? Lowered(a) == Lowered(b) : a == b;
    }

    bool MatchesAttribute(const CssSimpleSelector &simple, const CssElement &element)
    {
      std::string value;
      if (!element.GetCssAttribute(simple.name, value)) { return false; }

      const std::string wanted = simple.ignores_case ? Lowered(simple.value) : simple.value;
      if (simple.ignores_case) { value = Lowered(value); }

      switch (simple.compare)
      {
        case CssSimpleSelector::Compare::Exists:
          return true;
        case CssSimpleSelector::Compare::Equals:
          return value == wanted;
        case CssSimpleSelector::Compare::Word:
        {
          if (wanted.empty() || std::ranges::any_of(wanted, IsSpace)) { return false; }

          std::size_t start = 0;
          while (start <= value.size())
          {
            std::size_t end = start;
            while (end < value.size() && !IsSpace(value[end])) { end++; }

            if (value.substr(start, end - start) == wanted) { return true; }
            start = end + 1;
          }
          return false;
        }
        case CssSimpleSelector::Compare::Dash:
          return value == wanted || value.starts_with(wanted + "-");
        case CssSimpleSelector::Compare::Prefix:
          return !wanted.empty() && value.starts_with(wanted);
        case CssSimpleSelector::Compare::Suffix:
          return !wanted.empty() && value.ends_with(wanted);
        case CssSimpleSelector::Compare::Contains:
          return !wanted.empty() && value.find(wanted) != std::string::npos;
      }

      return false;
    }

    bool MatchesNth(const int step, const int offset, const std::size_t index)
    {
      const int place = static_cast<int>(index);

      if (step == 0) { return place == offset; }

      // there is a whole number that is not below 0 with step * n + offset
      // being the place
      const int distance = place - offset;
      return distance % step == 0 && distance / step >= 0;
    }

    bool MatchesFrom(
      const CssComplexSelector &selector,
      std::size_t index,
      const CssElement &element,
      const CssElement *scope);

    bool MatchesSimple(const CssSimpleSelector &simple, const CssElement &element, const CssElement *scope)
    {
      switch (simple.kind)
      {
        case CssSimpleSelector::Kind::Class:
          return element.HasCssClass(simple.name);
        case CssSimpleSelector::Kind::Id:
          return !simple.name.empty() && element.GetCssId() == simple.name;
        case CssSimpleSelector::Kind::Attribute:
          return MatchesAttribute(simple, element);
        case CssSimpleSelector::Kind::PseudoClass:
          break;
      }

      const std::string &name = simple.name;

      if (name == "not" || name == "is" || name == "where")
      {
        const bool any = std::ranges::any_of(simple.arguments, [&](const auto &argument)
        {
          return !argument->compounds.empty() &&
                 MatchesFrom(*argument, argument->compounds.size() - 1, element, scope);
        });

        return name == "not" ? !any : any;
      }

      if (name == "root") { return element.GetCssParent() == nullptr; }
      if (name == "scope") { return scope != nullptr ? &element == scope : element.GetCssParent() == nullptr; }
      if (name == "first-child") { return element.GetCssIndex() == 1; }
      if (name == "last-child") { return element.GetCssIndex() == element.GetCssSiblingCount(); }
      if (name == "only-child") { return element.GetCssSiblingCount() == 1; }
      if (name == "empty") { return !element.HasCssChildren(); }
      if (name == "nth-child") { return MatchesNth(simple.step, simple.offset, element.GetCssIndex()); }

      if (name == "nth-last-child")
      {
        return MatchesNth(
          simple.step, simple.offset, element.GetCssSiblingCount() - element.GetCssIndex() + 1);
      }

      return element.IsInCssState(name);
    }

    bool MatchesCompound(const CssCompoundSelector &compound, const CssElement &element, const CssElement *scope)
    {
      if (!compound.type.empty() && compound.type != element.GetCssType()) { return false; }

      return std::ranges::all_of(compound.parts, [&](const CssSimpleSelector &simple)
      {
        return MatchesSimple(simple, element, scope);
      });
    }

    bool MatchesFrom(
      const CssComplexSelector &selector,
      const std::size_t index,
      const CssElement &element,
      const CssElement *scope)
    {
      const CssCompoundSelector &compound = selector.compounds[index];
      if (!MatchesCompound(compound, element, scope)) { return false; }

      if (index == 0) { return true; }

      switch (compound.combinator)
      {
        case CssCombinator::Child:
        {
          const CssElement *parent = element.GetCssParent();
          return parent != nullptr && MatchesFrom(selector, index - 1, *parent, scope);
        }
        case CssCombinator::Descendant:
        {
          for (const CssElement *above = element.GetCssParent(); above != nullptr; above = above->GetCssParent())
          {
            if (MatchesFrom(selector, index - 1, *above, scope)) { return true; }
          }
          return false;
        }
        case CssCombinator::Adjacent:
        {
          const CssElement *before = element.GetCssPreviousSibling();
          return before != nullptr && MatchesFrom(selector, index - 1, *before, scope);
        }
        case CssCombinator::Sibling:
        {
          for (const CssElement *before = element.GetCssPreviousSibling();
               before != nullptr;
               before = before->GetCssPreviousSibling())
          {
            if (MatchesFrom(selector, index - 1, *before, scope)) { return true; }
          }
          return false;
        }
        default:
          return false;
      }
    }

    void AddUnique(std::vector<std::string> &names, const std::string &name)
    {
      if (!Contains(names, name)) { names.push_back(name); }
    }

    void Collect(
      const CssComplexSelector &selector,
      bool outer_above,
      bool outer_before,
      CssDependencies &dependencies);

    void CollectCompound(
      const CssCompoundSelector &compound,
      const bool above,
      const bool before,
      CssDependencies &dependencies)
    {
      for (const auto &simple : compound.parts)
      {
        switch (simple.kind)
        {
          case CssSimpleSelector::Kind::Class:
          case CssSimpleSelector::Kind::Id:
          case CssSimpleSelector::Kind::Attribute:
            if (above) { dependencies.classes_above = true; }
            if (before) { dependencies.classes_before = true; }
            break;
          case CssSimpleSelector::Kind::PseudoClass:
            if (Contains(state_names, simple.name))
            {
              // what can be used and what cannot are one state
              const std::string state = simple.name == "enabled" ? "disabled" : simple.name;
              if (above) { AddUnique(dependencies.states_above, state); }
              if (before) { AddUnique(dependencies.states_before, state); }
            } else if (Contains(structural_names, simple.name))
            {
              dependencies.structure = true;
            }

            for (const auto &argument : simple.arguments) { Collect(*argument, above, before, dependencies); }
            break;
        }
      }
    }

    void Collect(
      const CssComplexSelector &selector,
      const bool outer_above,
      const bool outer_before,
      CssDependencies &dependencies)
    {
      for (std::size_t i = 0; i < selector.compounds.size(); i++)
      {
        bool above = outer_above;
        bool before = outer_before;

        // what stands between this compound and the one the selector is
        // about
        for (std::size_t k = i + 1; k < selector.compounds.size(); k++)
        {
          const CssCombinator combinator = selector.compounds[k].combinator;

          if (combinator == CssCombinator::Child || combinator == CssCombinator::Descendant) { above = true; }

          if (combinator == CssCombinator::Adjacent || combinator == CssCombinator::Sibling)
          {
            before = true;
            dependencies.structure = true;
          }
        }

        CollectCompound(selector.compounds[i], above, before, dependencies);
      }
    }
  }

  bool ParseCssSelectors(
    const std::string &text,
    std::vector<CssComplexSelector> &selectors,
    std::string &error)
  {
    return ParseList(text, selectors, error, true);
  }

  bool MatchesCss(const CssComplexSelector &selector, const CssElement &element)
  {
    return MatchesCss(selector, element, nullptr);
  }

  bool MatchesCss(const CssComplexSelector &selector, const CssElement &element, const CssElement *scope)
  {
    if (selector.compounds.empty()) { return false; }

    return MatchesFrom(selector, selector.compounds.size() - 1, element, scope);
  }

  void CssDependencies::Add(const CssComplexSelector &selector)
  {
    Collect(selector, false, false, *this);
  }

  bool CssDependencies::HasStateAbove(const std::string &name) const
  {
    return Contains(states_above, name);
  }

  bool CssDependencies::HasStateBefore(const std::string &name) const
  {
    return Contains(states_before, name);
  }
} // neon
