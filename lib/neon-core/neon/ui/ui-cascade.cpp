#include "ui-cascade.hpp"

#include <algorithm>
#include <format>
#include <functional>

#include "ui-declarations.hpp"
#include "ui-properties.hpp"

namespace neon
{
  // Helpers of UiCascadeSettings, for this file alone.
  namespace
  {
    // in the order a later one wins over an earlier one
    const std::vector<std::string> state_names = {"focus", "hover", "active", "disabled"};

    bool Has(const UiStates &states, const std::string &name)
    {
      if (name == "focus") { return states.focus; }
      if (name == "hover") { return states.hover; }
      if (name == "active") { return states.active; }
      return states.disabled;
    }

    /// The rules that are about the element, or about a part of it, the
    /// one that counts least in front.
    std::vector<const UiStyleSheets::Rule *> Matching(
      const UiElement &element,
      const std::string &part,
      const UiStyleSheets *sheets)
    {
      std::vector<const UiStyleSheets::Rule *> rules;
      if (sheets == nullptr) { return rules; }

      for (const auto &rule : sheets->GetRules())
      {
        if (rule.selector.pseudo_element != part) { continue; }
        if (!sheets->Holds(rule.media)) { continue; }
        if (!MatchesCss(rule.selector, element)) { continue; }

        rules.push_back(&rule);
      }

      std::ranges::stable_sort(rules, [](const UiStyleSheets::Rule *a, const UiStyleSheets::Rule *b)
      {
        if (a->selector.specificity == b->selector.specificity) { return a->order < b->order; }
        return a->selector.specificity < b->selector.specificity;
      });

      return rules;
    }

    void Mix(std::size_t &fingerprint, const std::string &text)
    {
      fingerprint ^= std::hash<std::string>{}(text) + 0x9e3779b97f4a7c15ull + (fingerprint << 6) + (fingerprint >> 2);
    }

    /// Goes through everything that is written for an element, in the
    /// order of the cascade.
    class Pass
    {
      const UiElement &_element;
      const UiCascadeSettings &_settings;
      const std::vector<const UiStyleSheets::Rule *> &_rules;
      UiStates _states;

      void Report(const std::string &problem) const
      {
        if (_settings.problems != nullptr) { _settings.problems->push_back(problem); }
      }

    public:
      Pass(
        const UiElement &element,
        const UiCascadeSettings &settings,
        const std::vector<const UiStyleSheets::Rule *> &rules)
        : _element(element), _settings(settings), _rules(rules)
      {
        _states = UiCascade::DrawnStates(element.GetStates());
      }

      /// The custom properties the element sets, on top of those it takes
      /// from the element above.
      [[nodiscard]] std::shared_ptr<const UiElement::Variables> Variables(
        const std::shared_ptr<const UiElement::Variables> &inherited) const
      {
        std::vector<std::pair<std::string, std::string>> set;

        const auto of_rules = [&](const bool important)
        {
          for (const auto *rule : _rules)
          {
            for (const auto &declaration : *rule->declarations)
            {
              if (declaration.IsCustomProperty() && declaration.is_important == important)
              {
                set.emplace_back(declaration.name, declaration.value);
              }
            }
          }
        };

        of_rules(false);

        for (const auto &[name, value] : _element.GetWritten().GetEntries())
        {
          if (!name.starts_with("--")) { continue; }

          std::string text;
          if (float number = 0.0f; value.GetNumber(number)) { text = FormatCssNumber(number); }
          else { (void) value.GetText(text); }

          set.emplace_back(name, text);
        }

        for (const auto &[name, value] : _element.GetSetProperties())
        {
          if (name.starts_with("--")) { set.emplace_back(name, value); }
        }

        of_rules(true);

        if (set.empty()) { return inherited; }

        auto variables = std::make_shared<UiElement::Variables>();
        if (inherited != nullptr) { *variables = *inherited; }

        for (const auto &[name, value] : set) { (*variables)[name] = value; }
        return variables;
      }

      /// Puts what is written into a style. With `only`, nothing but the
      /// property of that name, as CSS writes it.
      void Run(
        const UiDeclarationContext &context,
        const std::string &only,
        UiStyle &style,
        std::size_t *fingerprint) const
      {
        const UiProperties &properties = UiProperties::Get();

        const auto declare = [&](
          const std::string &name,
          const std::string &value,
          const std::string &where,
          const std::string &from)
        {
          if (name.starts_with("--")) { return; }
          if (!only.empty() && name != only) { return; }

          std::vector<std::string> problems;
          if (!ApplyUiDeclaration(name, value, context, where, style, problems))
          {
            if (only.empty())
            {
              for (const auto &problem : problems) { Report(from + problem + ". The declaration is left out"); }
            }
            return;
          }

          // what the table of properties does not know is told apart by
          // what was written, so that a change of it is noticed
          if (fingerprint != nullptr && properties.Find(name) == nullptr)
          {
            Mix(*fingerprint, name);
            Mix(*fingerprint, value);
          }
        };

        const auto of_rules = [&](const bool important)
        {
          for (const auto *rule : _rules)
          {
            for (const auto &declaration : *rule->declarations)
            {
              if (declaration.is_important != important) { continue; }

              declare(
                declaration.name, declaration.value, rule->where,
                std::format("{}:{}: ", rule->sheet, declaration.line));
            }
          }
        };

        of_rules(false);

        // what the file writes for the element, and for the states it is in
        const DataValue &written = _element.GetWritten();
        const std::string yaml_only = UiProperties::ToYamlName(only);

        const auto of_map = [&](const DataValue &map)
        {
          if (!map.IsMap() || map.GetEntries().empty()) { return; }
          if (!only.empty() && map.Find(yaml_only) == nullptr) { return; }

          UiDeclarationContext of_file = context;
          of_file.reads_color_names = false;

          // what cannot be read was said when the file was read
          std::vector<std::string> unheard;

          DataValue one = DataValue::Map();
          if (!only.empty()) { one.Set(yaml_only, *map.Find(yaml_only)); }

          const UiPreparedProperties prepared = PrepareUiProperties(
            only.empty() ? map : one, of_file, false, _element.GetDocumentName(), _element.Describe(), unheard);

          ApplyUiProperties(prepared, of_file, _element.GetDocumentName(), _element.Describe(), style, unheard);

          if (only.empty())
          {
            for (const auto &problem : unheard)
            {
              // a name that is no property is none of the cascade's
              if (problem.find("is not known to") == std::string::npos) { Report(problem); }
            }
          }

          if (fingerprint != nullptr)
          {
            for (const auto &[name, value] : map.GetEntries())
            {
              if (!UiProperties::IsStyleName(name) || properties.Find(name) != nullptr) { continue; }

              std::string text;
              if (float number = 0.0f; value.GetNumber(number)) { text = FormatCssNumber(number); }
              else { (void) value.GetText(text); }

              Mix(*fingerprint, name);
              Mix(*fingerprint, text);
            }
          }
        };

        of_map(StyleOnly(written));

        for (const auto &state : state_names)
        {
          if (!Has(_states, state)) { continue; }

          if (const DataValue *map = written.Find(state); map != nullptr) { of_map(*map); }
        }

        for (const auto &[name, value] : _element.GetSetProperties())
        {
          declare(UiProperties::ToCssName(name), value, _element.Describe(), "");
        }

        of_rules(true);
      }

      /// What is written for the element, less what is no property.
      [[nodiscard]] static DataValue StyleOnly(const DataValue &written)
      {
        DataValue map = DataValue::Map();
        map.SetLine(written.GetLine());

        for (const auto &[name, value] : written.GetEntries())
        {
          if (UiProperties::IsStyleName(name)) { map.Set(name, value); }
        }

        return map;
      }
    };

    /// The element at the top of the tree an element is in.
    const UiElement &RootOf(const UiElement &element)
    {
      const UiElement *root = &element;
      while (root->GetParent() != nullptr) { root = root->GetParent(); }
      return *root;
    }
  }

  UiStates UiCascade::DrawnStates(const UiStates &states)
  {
    if (!states.disabled) { return states; }

    // no other state applies to what cannot be used
    UiStates drawn;
    drawn.disabled = true;
    drawn.checked = states.checked;
    return drawn;
  }

  void UiCascade::Compute(UiElement &element, const UiCascadeSettings &settings)
  {
    const UiElement *parent = element.GetParent();
    const UiStyle *parent_style = parent != nullptr ? &parent->GetStyle() : nullptr;

    const UiStyle initial;
    const float parent_font_size = parent_style != nullptr ? parent_style->font_size : initial.font_size;

    const auto rules = Matching(element, "", settings.sheets);
    const Pass pass(element, settings, rules);

    const auto variables = pass.Variables(parent != nullptr ? parent->GetVariables() : nullptr);

    // What the element starts with: what is inherited from the element
    // above, and what the engine gives an element of its kind.
    UiStyle style;

    if (parent_style != nullptr)
    {
      for (const auto &property : UiProperties::Get().GetAll())
      {
        if (property.is_inherited) { property.Copy(*parent_style, style); }
      }
    }

    element.ApplyDefaults(style);
    element.ApplyStateDefaults(style, DrawnStates(element.GetStates()));

    const UiStyle defaults = style;

    UiDeclarationContext context;
    context.parent = parent_style;
    context.defaults = &defaults;
    context.parent_font_size = parent_font_size;
    context.values.viewport_width = settings.viewport_width;
    context.values.viewport_height = settings.viewport_height;

    context.values.variable = [&variables](const std::string &name, std::string &value)
    {
      if (variables == nullptr) { return false; }

      const auto found = variables->find(name);
      if (found == variables->end()) { return false; }

      value = found->second;
      return true;
    };

    // The size of the font comes first, since an `em` of every other
    // property is what it comes to. For the element at the top, a `rem`
    // of the size of the font is that of the initial value.
    const bool is_root = parent == nullptr;
    context.values.font_size = parent_font_size;
    context.values.root_font_size = is_root ? initial.font_size : RootOf(element).GetStyle().font_size;

    UiStyle sized = style;
    pass.Run(context, "font-size", sized, nullptr);

    context.values.font_size = sized.font_size;
    if (is_root) { context.values.root_font_size = sized.font_size; }

    std::size_t fingerprint = 0;
    pass.Run(context, "", style, &fingerprint);

    // what ran in between must not have changed it
    style.font_size = sized.font_size;

    element.SetComputedStyle(style, variables, fingerprint);
  }

  UiDeclarationContext UiCascade::ContextOf(const UiElement &element, const UiCascadeSettings &settings)
  {
    const UiElement *parent = element.GetParent();
    const UiStyle initial;

    UiDeclarationContext context;
    context.parent = parent != nullptr ? &parent->GetStyle() : nullptr;
    context.parent_font_size = parent != nullptr ? parent->GetStyle().font_size : initial.font_size;
    context.values.font_size = element.GetComputedStyle().font_size;
    context.values.root_font_size = RootOf(element).GetComputedStyle().font_size;
    context.values.viewport_width = settings.viewport_width;
    context.values.viewport_height = settings.viewport_height;

    // kept by the function, so that it holds for as long as it is asked
    context.values.variable = [variables = element.GetVariables()](const std::string &name, std::string &value)
    {
      if (variables == nullptr) { return false; }

      const auto found = variables->find(name);
      if (found == variables->end()) { return false; }

      value = found->second;
      return true;
    };

    return context;
  }

  void UiCascade::ComputePart(
    const UiElement &element,
    const std::string &part,
    const UiCascadeSettings &settings,
    UiStyle &style)
  {
    const auto rules = Matching(element, part, settings.sheets);
    if (rules.empty()) { return; }

    const UiStyle &of_element = element.GetStyle();
    const UiStyle defaults = style;
    const auto variables = element.GetVariables();

    UiDeclarationContext context;
    context.parent = &of_element;
    context.defaults = &defaults;
    context.parent_font_size = of_element.font_size;
    context.values.font_size = of_element.font_size;
    context.values.root_font_size = RootOf(element).GetStyle().font_size;
    context.values.viewport_width = settings.viewport_width;
    context.values.viewport_height = settings.viewport_height;

    context.values.variable = [&variables](const std::string &name, std::string &value)
    {
      if (variables == nullptr) { return false; }

      const auto found = variables->find(name);
      if (found == variables->end()) { return false; }

      value = found->second;
      return true;
    };

    const std::string where_part = "::" + part;

    for (const bool important : {false, true})
    {
      for (const auto *rule : rules)
      {
        for (const auto &declaration : *rule->declarations)
        {
          if (declaration.IsCustomProperty() || declaration.is_important != important) { continue; }

          std::vector<std::string> problems;
          if (!ApplyUiDeclaration(declaration.name, declaration.value, context, rule->where, style, problems) &&
              settings.problems != nullptr)
          {
            for (const auto &problem : problems)
            {
              settings.problems->push_back(std::format(
                "{}:{}: {}. The declaration is left out", rule->sheet, declaration.line, problem));
            }
          }
        }
      }
    }

    (void) where_part;
  }
} // neon
