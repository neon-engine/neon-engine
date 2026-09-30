#include "css-selector.hpp"

#include <map>
#include <memory>
#include <set>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// What is expected here is what https://www.w3.org/TR/selectors-4/ says.

namespace
{
  using neon::CssCombinator;
  using neon::CssComplexSelector;
  using neon::CssDependencies;
  using neon::CssElement;
  using neon::CssSimpleSelector;
  using neon::CssSpecificity;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;
  using ::testing::UnorderedElementsAre;

  /// An element of a tree that a test builds.
  class Node final : public CssElement
  {
  public:
    std::string type;
    std::string id;
    std::set<std::string> classes;
    std::set<std::string> states;
    std::map<std::string, std::string> attributes;

    Node *parent = nullptr;
    std::vector<std::unique_ptr<Node>> children;

    Node &Add(const std::string &child_type, const std::string &child_id = "", const std::set<std::string> &of = {})
    {
      auto child = std::make_unique<Node>();
      child->type = child_type;
      child->id = child_id;
      child->classes = of;
      child->parent = this;

      children.push_back(std::move(child));
      return *children.back();
    }

    [[nodiscard]] const std::string &GetCssType() const override
    {
      return type;
    }

    [[nodiscard]] const std::string &GetCssId() const override
    {
      return id;
    }

    [[nodiscard]] bool HasCssClass(const std::string &name) const override
    {
      return classes.contains(name);
    }

    [[nodiscard]] bool GetCssAttribute(const std::string &name, std::string &value) const override
    {
      const auto found = attributes.find(name);
      if (found == attributes.end()) { return false; }

      value = found->second;
      return true;
    }

    [[nodiscard]] bool IsInCssState(const std::string &name) const override
    {
      if (name == "enabled") { return !states.contains("disabled"); }
      return states.contains(name);
    }

    [[nodiscard]] const CssElement *GetCssParent() const override
    {
      return parent;
    }

    [[nodiscard]] const CssElement *GetCssPreviousSibling() const override
    {
      if (parent == nullptr) { return nullptr; }

      const Node *before = nullptr;
      for (const auto &child : parent->children)
      {
        if (child.get() == this) { return before; }
        before = child.get();
      }
      return nullptr;
    }

    [[nodiscard]] std::size_t GetCssIndex() const override
    {
      if (parent == nullptr) { return 1; }

      for (std::size_t i = 0; i < parent->children.size(); i++)
      {
        if (parent->children[i].get() == this) { return i + 1; }
      }
      return 1;
    }

    [[nodiscard]] std::size_t GetCssSiblingCount() const override
    {
      return parent == nullptr ? 1 : parent->children.size();
    }

    [[nodiscard]] bool HasCssChildren() const override
    {
      return !children.empty();
    }
  };

  std::vector<CssComplexSelector> Read(const std::string &text)
  {
    std::vector<CssComplexSelector> selectors;
    std::string error;
    EXPECT_TRUE(neon::ParseCssSelectors(text, selectors, error)) << text << ": " << error;
    return selectors;
  }

  CssComplexSelector One(const std::string &text)
  {
    const auto selectors = Read(text);
    EXPECT_EQ(selectors.size(), 1u) << text;
    return selectors.empty() ? CssComplexSelector{} : selectors[0];
  }

  std::string ProblemOf(const std::string &text)
  {
    std::vector<CssComplexSelector> selectors;
    std::string error;
    EXPECT_FALSE(neon::ParseCssSelectors(text, selectors, error)) << text;
    EXPECT_TRUE(selectors.empty()) << text;
    return error;
  }

  CssSpecificity SpecificityOf(const std::string &text)
  {
    return One(text).specificity;
  }

  bool Matches(const std::string &text, const Node &node)
  {
    return std::ranges::any_of(Read(text), [&node](const CssComplexSelector &selector)
    {
      return neon::MatchesCss(selector, node);
    });
  }

  /// The tree the tests match against:
  ///
  ///     panel #menu .window
  ///       label #title .heading
  ///       panel #list .rows .inventory
  ///         button #first .row
  ///         button #second .row .selected
  ///         button #third .row
  ///         button #fourth .row
  ///       button #close .primary
  ///       input #name
  class CssSelectorMatchTest : public ::testing::Test
  {
  protected:
    Node _menu;
    Node *_title = nullptr;
    Node *_list = nullptr;
    Node *_first = nullptr;
    Node *_second = nullptr;
    Node *_third = nullptr;
    Node *_fourth = nullptr;
    Node *_close = nullptr;
    Node *_name = nullptr;

    void SetUp() override
    {
      _menu.type = "panel";
      _menu.id = "menu";
      _menu.classes = {"window"};

      _title = &_menu.Add("label", "title", {"heading"});
      _list = &_menu.Add("panel", "list", {"rows", "inventory"});
      _first = &_list->Add("button", "first", {"row"});
      _second = &_list->Add("button", "second", {"row", "selected"});
      _third = &_list->Add("button", "third", {"row"});
      _fourth = &_list->Add("button", "fourth", {"row"});
      _close = &_menu.Add("button", "close", {"primary"});
      _name = &_menu.Add("input", "name");

      _name->attributes = {{"kind", "text"}, {"placeholder", "Your name"}, {"lang", "en-GB"}};
    }

    /// The names of the elements a selector matches, from the top of the
    /// tree to its end.
    std::vector<std::string> Matched(const std::string &text)
    {
      std::vector<std::string> names;
      Collect(_menu, text, names);
      return names;
    }

  private:
    static void Collect(const Node &node, const std::string &text, std::vector<std::string> &names)
    {
      if (Matches(text, node)) { names.push_back(node.id); }
      for (const auto &child : node.children) { Collect(*child, text, names); }
    }
  };

  // how much a selector counts, with the examples of the specification

  TEST(CssSpecificityTest, CountsNothingForTheUniversalSelector)
  {
    EXPECT_EQ(SpecificityOf("*"), (CssSpecificity{0, 0, 0}));
  }

  TEST(CssSpecificityTest, CountsTypes)
  {
    EXPECT_EQ(SpecificityOf("LI"), (CssSpecificity{0, 0, 1}));
    EXPECT_EQ(SpecificityOf("UL LI"), (CssSpecificity{0, 0, 2}));
    EXPECT_EQ(SpecificityOf("UL OL+LI"), (CssSpecificity{0, 0, 3}));
  }

  TEST(CssSpecificityTest, CountsAttributesAndClasses)
  {
    EXPECT_EQ(SpecificityOf("H1 + *[REL=up]"), (CssSpecificity{0, 1, 1}));
    EXPECT_EQ(SpecificityOf("UL OL LI.red"), (CssSpecificity{0, 1, 3}));
    EXPECT_EQ(SpecificityOf("LI.red.level"), (CssSpecificity{0, 2, 1}));
  }

  TEST(CssSpecificityTest, CountsIdentifiers)
  {
    EXPECT_EQ(SpecificityOf("#x34y"), (CssSpecificity{1, 0, 0}));
  }

  TEST(CssSpecificityTest, CountsWhatIsInsideNotAndNotTheNotItself)
  {
    EXPECT_EQ(SpecificityOf("#s12:not(FOO)"), (CssSpecificity{1, 0, 1}));
  }

  TEST(CssSpecificityTest, CountsTheMostSpecificArgumentOfIsAndNot)
  {
    EXPECT_EQ(SpecificityOf(".foo :is(.bar, #baz)"), (CssSpecificity{1, 1, 0}));
    EXPECT_EQ(SpecificityOf(":is(em, #foo)"), (CssSpecificity{1, 0, 0}));
    EXPECT_EQ(SpecificityOf(":not(em, strong#foo)"), (CssSpecificity{1, 0, 1}));
  }

  TEST(CssSpecificityTest, CountsNothingForWhere)
  {
    EXPECT_EQ(SpecificityOf(".qux:where(em, #foo#bar#baz)"), (CssSpecificity{0, 1, 0}));
  }

  TEST(CssSpecificityTest, CountsAPseudoClassAsAClass)
  {
    EXPECT_EQ(SpecificityOf("button:hover"), (CssSpecificity{0, 1, 1}));
    EXPECT_EQ(SpecificityOf("button:hover:focus"), (CssSpecificity{0, 2, 1}));
    EXPECT_EQ(SpecificityOf("li:nth-child(2n+1)"), (CssSpecificity{0, 1, 1}));
    EXPECT_EQ(SpecificityOf(":root"), (CssSpecificity{0, 1, 0}));
  }

  TEST(CssSpecificityTest, CountsAPseudoElementAsAType)
  {
    EXPECT_EQ(SpecificityOf("panel::scrollbar-thumb"), (CssSpecificity{0, 0, 2}));
    EXPECT_EQ(SpecificityOf("::placeholder"), (CssSpecificity{0, 0, 1}));
  }

  TEST(CssSpecificityTest, ComparesIdentifiersFirstThenClassesThenTypes)
  {
    EXPECT_TRUE((CssSpecificity{0, 9, 9}) < (CssSpecificity{1, 0, 0}));
    EXPECT_TRUE((CssSpecificity{0, 0, 9}) < (CssSpecificity{0, 1, 0}));
    EXPECT_TRUE((CssSpecificity{0, 1, 0}) < (CssSpecificity{0, 1, 1}));
    EXPECT_FALSE((CssSpecificity{1, 0, 0}) < (CssSpecificity{0, 9, 9}));
    EXPECT_FALSE((CssSpecificity{1, 2, 3}) < (CssSpecificity{1, 2, 3}));
  }

  // what is read

  TEST(CssSelectorParseTest, ReadsACompoundOfEveryKind)
  {
    const CssComplexSelector selector = One("button#start.primary.large[kind=text]:hover");

    ASSERT_EQ(selector.compounds.size(), 1u);
    EXPECT_EQ(selector.compounds[0].type, "button");
    EXPECT_EQ(selector.compounds[0].combinator, CssCombinator::None);

    const auto &parts = selector.compounds[0].parts;
    ASSERT_EQ(parts.size(), 5u);
    EXPECT_EQ(parts[0].kind, CssSimpleSelector::Kind::Id);
    EXPECT_EQ(parts[0].name, "start");
    EXPECT_EQ(parts[1].kind, CssSimpleSelector::Kind::Class);
    EXPECT_EQ(parts[1].name, "primary");
    EXPECT_EQ(parts[2].name, "large");
    EXPECT_EQ(parts[3].kind, CssSimpleSelector::Kind::Attribute);
    EXPECT_EQ(parts[3].name, "kind");
    EXPECT_EQ(parts[3].value, "text");
    EXPECT_EQ(parts[4].kind, CssSimpleSelector::Kind::PseudoClass);
    EXPECT_EQ(parts[4].name, "hover");
  }

  TEST(CssSelectorParseTest, ReadsEveryCombinator)
  {
    const CssComplexSelector selector = One("a b > c + d ~ e");

    ASSERT_EQ(selector.compounds.size(), 5u);
    EXPECT_EQ(selector.compounds[0].combinator, CssCombinator::None);
    EXPECT_EQ(selector.compounds[1].combinator, CssCombinator::Descendant);
    EXPECT_EQ(selector.compounds[2].combinator, CssCombinator::Child);
    EXPECT_EQ(selector.compounds[3].combinator, CssCombinator::Adjacent);
    EXPECT_EQ(selector.compounds[4].combinator, CssCombinator::Sibling);
    EXPECT_EQ(selector.compounds[4].type, "e");
  }

  TEST(CssSelectorParseTest, ReadsCombinatorsWithoutSpaces)
  {
    const CssComplexSelector selector = One("a>b+c~d");

    ASSERT_EQ(selector.compounds.size(), 4u);
    EXPECT_EQ(selector.compounds[1].combinator, CssCombinator::Child);
    EXPECT_EQ(selector.compounds[2].combinator, CssCombinator::Adjacent);
    EXPECT_EQ(selector.compounds[3].combinator, CssCombinator::Sibling);
  }

  TEST(CssSelectorParseTest, ReadsAListAndKeepsWhatWasWritten)
  {
    const auto selectors = Read("  button ,\n .row   >  label,#close  ");

    ASSERT_EQ(selectors.size(), 3u);
    EXPECT_EQ(selectors[0].text, "button");
    EXPECT_EQ(selectors[1].text, ".row > label");
    EXPECT_EQ(selectors[2].text, "#close");
  }

  TEST(CssSelectorParseTest, ReadsAPseudoElement)
  {
    const CssComplexSelector selector = One("panel.list::scrollbar-thumb");

    EXPECT_EQ(selector.pseudo_element, "scrollbar-thumb");
    ASSERT_EQ(selector.compounds.size(), 1u);
    EXPECT_EQ(selector.compounds[0].type, "panel");
  }

  TEST(CssSelectorParseTest, TakesTheNamesBrowsersHaveForTheScrollbar)
  {
    EXPECT_EQ(One("::-webkit-scrollbar-thumb").pseudo_element, "scrollbar-thumb");
  }

  TEST(CssSelectorParseTest, ReadsANameThatStartsWithADigitBehindAHash)
  {
    const CssComplexSelector selector = One("#1st");
    EXPECT_EQ(selector.compounds[0].parts[0].name, "1st");
  }

  TEST(CssSelectorParseTest, ReadsNamesWithUnderscoresAndHyphens)
  {
    const CssComplexSelector selector = One("mini_map.is-open#the_map-2");

    EXPECT_EQ(selector.compounds[0].type, "mini_map");
    EXPECT_EQ(selector.compounds[0].parts[0].name, "is-open");
    EXPECT_EQ(selector.compounds[0].parts[1].name, "the_map-2");
  }

  TEST(CssSelectorParseTest, ReadsEveryWayToCompareAnAttribute)
  {
    using Compare = CssSimpleSelector::Compare;

    const std::pair<const char *, Compare> cases[] = {
      {"[a]", Compare::Exists},
      {"[a=b]", Compare::Equals},
      {"[a~=b]", Compare::Word},
      {"[a|=b]", Compare::Dash},
      {"[a^=b]", Compare::Prefix},
      {"[a$=b]", Compare::Suffix},
      {"[a*=b]", Compare::Contains}
    };

    for (const auto &[text, compare] : cases)
    {
      const CssComplexSelector selector = One(text);
      ASSERT_EQ(selector.compounds[0].parts.size(), 1u) << text;
      EXPECT_EQ(selector.compounds[0].parts[0].compare, compare) << text;
      EXPECT_EQ(selector.compounds[0].parts[0].name, "a") << text;
    }
  }

  TEST(CssSelectorParseTest, ReadsTheValueOfAnAttributeInQuotesAndWithSpacesAround)
  {
    const CssSimpleSelector simple = One("[ placeholder = \"Your name\" i ]").compounds[0].parts[0];

    EXPECT_EQ(simple.name, "placeholder");
    EXPECT_EQ(simple.value, "Your name");
    EXPECT_TRUE(simple.ignores_case);
  }

  TEST(CssSelectorParseTest, WritesAnAttributeWithAnUnderscoreWhereCssHasAHyphen)
  {
    EXPECT_EQ(One("[max-length]").compounds[0].parts[0].name, "max_length");
    EXPECT_EQ(One("[max_length]").compounds[0].parts[0].name, "max_length");
  }

  TEST(CssSelectorParseTest, ReadsAnPlusB)
  {
    const std::tuple<const char *, int, int> cases[] = {
      {":nth-child(odd)", 2, 1},
      {":nth-child(even)", 2, 0},
      {":nth-child(EVEN)", 2, 0},
      {":nth-child(3)", 0, 3},
      {":nth-child(n)", 1, 0},
      {":nth-child(2n)", 2, 0},
      {":nth-child(2n+1)", 2, 1},
      {":nth-child( 2n + 1 )", 2, 1},
      {":nth-child(3n-2)", 3, -2},
      {":nth-child(-n+3)", -1, 3},
      {":nth-child(+n)", 1, 0},
      {":nth-child(+3n - 2)", 3, -2},
      {":nth-child(-2n+6)", -2, 6},
      {":nth-child(0n+5)", 0, 5},
      {":nth-last-child(2)", 0, 2}
    };

    for (const auto &[text, step, offset] : cases)
    {
      const CssSimpleSelector simple = One(text).compounds[0].parts[0];
      EXPECT_EQ(simple.step, step) << text;
      EXPECT_EQ(simple.offset, offset) << text;
    }
  }

  TEST(CssSelectorParseTest, TakesFocusVisibleForFocus)
  {
    EXPECT_EQ(One("button:focus-visible").compounds[0].parts[0].name, "focus");
  }

  TEST(CssSelectorParseTest, ReadsPseudoClassesInAnyCase)
  {
    EXPECT_EQ(One("button:HOVER").compounds[0].parts[0].name, "hover");
  }

  TEST(CssSelectorParseTest, SaysWhatIsWrongWithASelector)
  {
    EXPECT_EQ(ProblemOf(""), "there is no selector");
    EXPECT_EQ(ProblemOf("   "), "there is no selector");
    EXPECT_EQ(ProblemOf("button,"), "a selector is empty");
    EXPECT_EQ(ProblemOf(",button"), "a selector is empty");
    EXPECT_EQ(ProblemOf("a >"), "a selector ends with a combinator, where a selector was expected behind it");
    EXPECT_EQ(ProblemOf("> a"), "'>' cannot start a selector");
    EXPECT_EQ(ProblemOf("a > > b"), "'>' cannot start a selector");
    EXPECT_EQ(ProblemOf("."), "'.' is followed by no name of a class");
    EXPECT_EQ(ProblemOf("a.5"), "'.' is followed by no name of a class");
    EXPECT_EQ(ProblemOf("#"), "'#' is followed by no name of an element");
    EXPECT_EQ(ProblemOf("a:"), "':' is followed by no name of a pseudo-class");
    EXPECT_EQ(ProblemOf("a::"), "'::' is followed by no name of a pseudo-element");
    EXPECT_EQ(ProblemOf("a$b"), "'$' cannot be part of a selector");
    EXPECT_EQ(ProblemOf("[=b]"), "'[' is followed by no name of an attribute");
    EXPECT_EQ(ProblemOf("[a"), "'[a' is not closed");
    EXPECT_EQ(ProblemOf("[a?b]"), "'[a' is followed by '?', where ], =, ~=, |=, ^=, $=, or *= was expected");
    EXPECT_EQ(ProblemOf("[a=]"), "'[a' compares with no value");
    EXPECT_EQ(ProblemOf("[a=b"), "'[a' is not closed");
    EXPECT_EQ(ProblemOf("[a~b]"), "'[a' has an operator without its '='");
    EXPECT_EQ(ProblemOf("[a=\"b]"), "a text in quotes is not closed");
    EXPECT_EQ(ProblemOf("a:hover(b)"), "':hover' takes nothing in brackets");
    EXPECT_EQ(ProblemOf("a:not"), "':not' has no selector in brackets");
    EXPECT_EQ(ProblemOf("a:not(b"), "':not(' is not closed");
    EXPECT_EQ(ProblemOf("a:not()"), "':not()' cannot be read: there is no selector");
    EXPECT_EQ(ProblemOf("a:not(::part)"), "':not(::part)' cannot be read: '::part' cannot be written in brackets");
    EXPECT_EQ(
      ProblemOf("a:nth-child(two)"),
      "':nth-child(two)' cannot be read, where an+b was expected, such as 2n+1, odd, or 3");
    EXPECT_EQ(
      ProblemOf("a:nth-child"),
      "':nth-child()' cannot be read, where an+b was expected, such as 2n+1, odd, or 3");
    EXPECT_EQ(
      ProblemOf("a:nth-child(2n+)"),
      "':nth-child(2n+)' cannot be read, where an+b was expected, such as 2n+1, odd, or 3");
    EXPECT_EQ(
      ProblemOf("a::before:hover"),
      "'::before' is followed by ':', where the end of the selector was expected");
    EXPECT_EQ(
      ProblemOf("a::before b"),
      "'::before' is followed by more, where the end of the selector was expected");
  }

  TEST(CssSelectorParseTest, RefusesAPseudoClassItDoesNotKnow)
  {
    EXPECT_EQ(
      ProblemOf("a:visited"),
      "':visited' is not a pseudo-class that is known. Known are: hover, active, focus, focus-visible, "
      "focus-within, disabled, enabled, checked, valid, invalid, first-child, last-child, only-child, "
      "nth-child(), nth-last-child(), empty, root, scope, not(), is(), where()");
  }

  TEST(CssSelectorParseTest, RefusesAWholeListForOneSelectorThatIsWrong)
  {
    EXPECT_EQ(ProblemOf("button, a:visited, label").substr(0, 10), "':visited'");
  }

  // what is matched

  TEST_F(CssSelectorMatchTest, MatchesEveryElementWithTheUniversalSelector)
  {
    EXPECT_THAT(
      Matched("*"),
      ElementsAre("menu", "title", "list", "first", "second", "third", "fourth", "close", "name"));
  }

  TEST_F(CssSelectorMatchTest, MatchesByType)
  {
    EXPECT_THAT(Matched("button"), ElementsAre("first", "second", "third", "fourth", "close"));
    EXPECT_THAT(Matched("panel"), ElementsAre("menu", "list"));
    EXPECT_THAT(Matched("slider"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, TellsTypesApartByCase)
  {
    EXPECT_THAT(Matched("Button"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesByClass)
  {
    EXPECT_THAT(Matched(".row"), ElementsAre("first", "second", "third", "fourth"));
    EXPECT_THAT(Matched(".row.selected"), ElementsAre("second"));
    EXPECT_THAT(Matched(".selected.row"), ElementsAre("second"));
    EXPECT_THAT(Matched(".rows.row"), IsEmpty());
    EXPECT_THAT(Matched("*.primary"), ElementsAre("close"));
  }

  TEST_F(CssSelectorMatchTest, MatchesByName)
  {
    EXPECT_THAT(Matched("#close"), ElementsAre("close"));
    EXPECT_THAT(Matched("button#close"), ElementsAre("close"));
    EXPECT_THAT(Matched("label#close"), IsEmpty());
    EXPECT_THAT(Matched("#missing"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesAList)
  {
    EXPECT_THAT(Matched("label, #close, .selected"), ElementsAre("title", "second", "close"));
  }

  TEST_F(CssSelectorMatchTest, MatchesWhatIsSomewhereInside)
  {
    EXPECT_THAT(Matched("#menu button"), ElementsAre("first", "second", "third", "fourth", "close"));
    EXPECT_THAT(Matched("#list button"), ElementsAre("first", "second", "third", "fourth"));
    EXPECT_THAT(Matched(".window .rows .row"), ElementsAre("first", "second", "third", "fourth"));
    EXPECT_THAT(Matched("#list #menu"), IsEmpty());

    // an element is not inside itself
    EXPECT_THAT(Matched("#list #list"), IsEmpty());
    EXPECT_THAT(Matched("panel panel"), ElementsAre("list"));
  }

  TEST_F(CssSelectorMatchTest, MatchesWhatIsDirectlyInside)
  {
    EXPECT_THAT(Matched("#menu > button"), ElementsAre("close"));
    EXPECT_THAT(Matched("#list > button"), ElementsAre("first", "second", "third", "fourth"));
    EXPECT_THAT(Matched("#menu > #list > .selected"), ElementsAre("second"));
    EXPECT_THAT(Matched("#menu > .selected"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesWhatIsDirectlyBehind)
  {
    EXPECT_THAT(Matched(".selected + button"), ElementsAre("third"));
    EXPECT_THAT(Matched("label + panel"), ElementsAre("list"));
    EXPECT_THAT(Matched("button + button"), ElementsAre("second", "third", "fourth"));
    EXPECT_THAT(Matched("label + button"), IsEmpty());

    // not across parents
    EXPECT_THAT(Matched("#fourth + button"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesWhatIsSomewhereBehind)
  {
    EXPECT_THAT(Matched(".selected ~ button"), ElementsAre("third", "fourth"));
    EXPECT_THAT(Matched("label ~ *"), ElementsAre("list", "close", "name"));
    EXPECT_THAT(Matched("#fourth ~ button"), IsEmpty());

    // not what is in front
    EXPECT_THAT(Matched(".selected ~ #first"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesCombinatorsTogether)
  {
    EXPECT_THAT(Matched("label ~ panel > .selected + button"), ElementsAre("third"));
    EXPECT_THAT(Matched("label + panel button"), ElementsAre("first", "second", "third", "fourth"));
    EXPECT_THAT(Matched(".window > label ~ button"), ElementsAre("close"));
  }

  TEST_F(CssSelectorMatchTest, GoesBackWhenTheNearestElementAboveDoesNotFit)
  {
    // the nearest panel above #first is #list, which is not directly
    // inside nothing. The one above that is
    Node &outer = *_list;
    Node &inner = outer.Add("panel", "inner");
    Node &deep = inner.Add("label", "deep");

    EXPECT_TRUE(Matches("#menu > panel label", deep));
    EXPECT_TRUE(Matches("#list label", deep));
    EXPECT_FALSE(Matches("#list > label", deep));
  }

  TEST_F(CssSelectorMatchTest, MatchesAnAttribute)
  {
    EXPECT_THAT(Matched("[kind]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[kind=text]"), ElementsAre("name"));
    EXPECT_THAT(Matched("input[kind=\"text\"]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[kind=password]"), IsEmpty());
    EXPECT_THAT(Matched("[missing]"), IsEmpty());
    EXPECT_THAT(Matched("[kind=Text]"), IsEmpty());
    EXPECT_THAT(Matched("[kind=Text i]"), ElementsAre("name"));
  }

  TEST_F(CssSelectorMatchTest, ComparesAnAttributeInEveryWay)
  {
    EXPECT_THAT(Matched("[placeholder~=name]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[placeholder~=Your]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[placeholder~=nam]"), IsEmpty());

    EXPECT_THAT(Matched("[lang|=en]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[lang|=en-GB]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[lang|=e]"), IsEmpty());

    EXPECT_THAT(Matched("[placeholder^=Your]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[placeholder^=name]"), IsEmpty());

    EXPECT_THAT(Matched("[placeholder$=name]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[placeholder$=Your]"), IsEmpty());

    EXPECT_THAT(Matched("[placeholder*=\"r n\"]"), ElementsAre("name"));
    EXPECT_THAT(Matched("[placeholder*=xyz]"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesNoAttributeWithAnEmptyValueToLookFor)
  {
    // as the specification says for ^=, $=, and *=
    EXPECT_THAT(Matched("[placeholder^=\"\"]"), IsEmpty());
    EXPECT_THAT(Matched("[placeholder$=\"\"]"), IsEmpty());
    EXPECT_THAT(Matched("[placeholder*=\"\"]"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesTheStatesOfAnElement)
  {
    _second->states = {"hover", "focus"};
    _third->states = {"active"};
    _close->states = {"disabled"};
    _list->states = {"focus-within", "hover"};

    EXPECT_THAT(Matched("button:hover"), ElementsAre("second"));
    EXPECT_THAT(Matched(":hover"), ElementsAre("list", "second"));
    EXPECT_THAT(Matched(":focus"), ElementsAre("second"));
    EXPECT_THAT(Matched(":focus-visible"), ElementsAre("second"));
    EXPECT_THAT(Matched(":focus-within"), ElementsAre("list"));
    EXPECT_THAT(Matched(":active"), ElementsAre("third"));
    EXPECT_THAT(Matched(":disabled"), ElementsAre("close"));
    EXPECT_THAT(Matched("button:enabled"), ElementsAre("first", "second", "third", "fourth"));
    EXPECT_THAT(Matched("button:hover:focus"), ElementsAre("second"));
    EXPECT_THAT(Matched("button:hover:active"), IsEmpty());
    EXPECT_THAT(Matched(":checked"), IsEmpty());

    _first->states = {"checked"};
    EXPECT_THAT(Matched(":checked"), ElementsAre("first"));
  }

  TEST_F(CssSelectorMatchTest, MatchesAStateOfAnElementAboveAndInFront)
  {
    _list->states = {"hover"};
    _second->states = {"focus"};

    EXPECT_THAT(Matched("#list:hover > button"), ElementsAre("first", "second", "third", "fourth"));
    EXPECT_THAT(Matched("button:focus + button"), ElementsAre("third"));
    EXPECT_THAT(Matched("button:focus ~ button"), ElementsAre("third", "fourth"));
  }

  TEST_F(CssSelectorMatchTest, MatchesTheElementAtTheTopAsRoot)
  {
    EXPECT_THAT(Matched(":root"), ElementsAre("menu"));
    EXPECT_THAT(Matched(":root > label"), ElementsAre("title"));
    EXPECT_THAT(Matched("panel:root"), ElementsAre("menu"));
    EXPECT_THAT(Matched("label:root"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesTheFirstAndTheLastUnderAParent)
  {
    EXPECT_THAT(Matched("button:first-child"), ElementsAre("first"));
    EXPECT_THAT(Matched("button:last-child"), ElementsAre("fourth"));
    EXPECT_THAT(Matched("#menu > :first-child"), ElementsAre("title"));
    EXPECT_THAT(Matched("#menu > :last-child"), ElementsAre("name"));

    // the element at the top is the only one where it is
    EXPECT_TRUE(Matches(":first-child", _menu));
    EXPECT_TRUE(Matches(":last-child", _menu));
    EXPECT_TRUE(Matches(":only-child", _menu));
  }

  TEST_F(CssSelectorMatchTest, MatchesAnOnlyChild)
  {
    Node &alone = _title->Add("image", "icon");

    EXPECT_TRUE(Matches(":only-child", alone));
    EXPECT_FALSE(Matches(":only-child", *_first));
  }

  TEST_F(CssSelectorMatchTest, MatchesByThePlaceUnderAParent)
  {
    EXPECT_THAT(Matched("#list > :nth-child(1)"), ElementsAre("first"));
    EXPECT_THAT(Matched("#list > :nth-child(3)"), ElementsAre("third"));
    EXPECT_THAT(Matched("#list > :nth-child(5)"), IsEmpty());
    EXPECT_THAT(Matched("#list > :nth-child(0)"), IsEmpty());

    EXPECT_THAT(Matched("#list > :nth-child(odd)"), ElementsAre("first", "third"));
    EXPECT_THAT(Matched("#list > :nth-child(even)"), ElementsAre("second", "fourth"));
    EXPECT_THAT(Matched("#list > :nth-child(2n+1)"), ElementsAre("first", "third"));
    EXPECT_THAT(Matched("#list > :nth-child(2n)"), ElementsAre("second", "fourth"));
    EXPECT_THAT(Matched("#list > :nth-child(n)"), ElementsAre("first", "second", "third", "fourth"));
    EXPECT_THAT(Matched("#list > :nth-child(n+3)"), ElementsAre("third", "fourth"));
    EXPECT_THAT(Matched("#list > :nth-child(-n+2)"), ElementsAre("first", "second"));
    EXPECT_THAT(Matched("#list > :nth-child(3n+1)"), ElementsAre("first", "fourth"));
    EXPECT_THAT(Matched("#list > :nth-child(3n-2)"), ElementsAre("first", "fourth"));
    EXPECT_THAT(Matched("#list > :nth-child(-2n+4)"), ElementsAre("second", "fourth"));
    EXPECT_THAT(Matched("#list > :nth-child(0n+2)"), ElementsAre("second"));
    EXPECT_THAT(Matched("#list > :nth-child(-n)"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, CountsFromTheEndForNthLastChild)
  {
    EXPECT_THAT(Matched("#list > :nth-last-child(1)"), ElementsAre("fourth"));
    EXPECT_THAT(Matched("#list > :nth-last-child(-n+2)"), ElementsAre("third", "fourth"));
    EXPECT_THAT(Matched("#list > :nth-last-child(odd)"), ElementsAre("second", "fourth"));
  }

  TEST_F(CssSelectorMatchTest, MatchesWhatHasNothingInside)
  {
    EXPECT_THAT(Matched("panel:empty"), IsEmpty());
    EXPECT_THAT(Matched("#menu > :empty"), ElementsAre("title", "close", "name"));
  }

  TEST_F(CssSelectorMatchTest, MatchesWhatASelectorDoesNotMatch)
  {
    EXPECT_THAT(Matched("button:not(.row)"), ElementsAre("close"));
    EXPECT_THAT(Matched(".row:not(.selected)"), ElementsAre("first", "third", "fourth"));
    EXPECT_THAT(Matched(".row:not(.selected, :last-child)"), ElementsAre("first", "third"));
    EXPECT_THAT(Matched("button:not(#list > *)"), ElementsAre("close"));
    EXPECT_THAT(Matched(".row:not(:not(.selected))"), ElementsAre("second"));

    _close->states = {"disabled"};
    EXPECT_THAT(Matched("#menu > button:not(:disabled)"), IsEmpty());
  }

  TEST_F(CssSelectorMatchTest, MatchesAnyOfTheSelectorsOfIsAndWhere)
  {
    EXPECT_THAT(Matched(":is(label, .primary)"), ElementsAre("title", "close"));
    EXPECT_THAT(Matched(":where(label, .primary)"), ElementsAre("title", "close"));
    EXPECT_THAT(Matched("#list > :is(:first-child, :last-child)"), ElementsAre("first", "fourth"));
  }

  TEST_F(CssSelectorMatchTest, MatchesTheElementAPartBelongsTo)
  {
    EXPECT_THAT(Matched("#list::scrollbar-thumb"), ElementsAre("list"));
  }

  TEST_F(CssSelectorMatchTest, MatchesTheElementASearchStartsAtAsScope)
  {
    const CssComplexSelector selector = One(":scope > button");

    EXPECT_TRUE(neon::MatchesCss(selector, *_first, _list));
    EXPECT_FALSE(neon::MatchesCss(selector, *_close, _list));
    EXPECT_TRUE(neon::MatchesCss(selector, *_close, &_menu));

    // without one, it is the element at the top
    EXPECT_TRUE(neon::MatchesCss(selector, *_close));
    EXPECT_FALSE(neon::MatchesCss(selector, *_first));
  }

  TEST_F(CssSelectorMatchTest, MatchesNothingWithASelectorThatWasNotRead)
  {
    EXPECT_FALSE(neon::MatchesCss(CssComplexSelector{}, _menu));
  }

  // what a selector depends on

  TEST(CssDependenciesTest, DependsOnNothingElseForASelectorAboutOneElement)
  {
    CssDependencies dependencies;
    dependencies.Add(One("button.primary:hover"));

    EXPECT_THAT(dependencies.states_above, IsEmpty());
    EXPECT_THAT(dependencies.states_before, IsEmpty());
    EXPECT_FALSE(dependencies.classes_above);
    EXPECT_FALSE(dependencies.classes_before);
    EXPECT_FALSE(dependencies.structure);
  }

  TEST(CssDependenciesTest, KnowsTheStatesAndClassesOfElementsAbove)
  {
    CssDependencies dependencies;
    dependencies.Add(One("panel:hover > button"));
    dependencies.Add(One(".open label:focus"));

    EXPECT_THAT(dependencies.states_above, ElementsAre("hover"));
    EXPECT_TRUE(dependencies.HasStateAbove("hover"));
    EXPECT_FALSE(dependencies.HasStateAbove("focus"));
    EXPECT_TRUE(dependencies.classes_above);
    EXPECT_FALSE(dependencies.classes_before);
    EXPECT_THAT(dependencies.states_before, IsEmpty());
  }

  TEST(CssDependenciesTest, KnowsTheStatesAndClassesOfElementsInFront)
  {
    CssDependencies dependencies;
    dependencies.Add(One("input:focus + label"));
    dependencies.Add(One(".selected ~ button"));

    EXPECT_THAT(dependencies.states_before, ElementsAre("focus"));
    EXPECT_TRUE(dependencies.HasStateBefore("focus"));
    EXPECT_TRUE(dependencies.classes_before);
    EXPECT_FALSE(dependencies.classes_above);
    EXPECT_TRUE(dependencies.structure);
  }

  TEST(CssDependenciesTest, KnowsBothForAnElementInFrontOfOneAbove)
  {
    CssDependencies dependencies;
    dependencies.Add(One("input:checked + panel > label"));

    EXPECT_THAT(dependencies.states_above, ElementsAre("checked"));
    EXPECT_THAT(dependencies.states_before, ElementsAre("checked"));
  }

  TEST(CssDependenciesTest, TakesWhatCanBeUsedAndWhatCannotForOneState)
  {
    CssDependencies dependencies;
    dependencies.Add(One("panel:enabled button"));

    EXPECT_TRUE(dependencies.HasStateAbove("disabled"));
  }

  TEST(CssDependenciesTest, KnowsWhenThePlaceAmongSiblingsMatters)
  {
    CssDependencies first;
    first.Add(One("button:first-child"));
    EXPECT_TRUE(first.structure);

    CssDependencies nth;
    nth.Add(One("li:nth-child(2n)"));
    EXPECT_TRUE(nth.structure);

    CssDependencies inside_not;
    inside_not.Add(One("li:not(:last-child)"));
    EXPECT_TRUE(inside_not.structure);
  }

  TEST(CssDependenciesTest, LooksIntoTheBracketsOfNot)
  {
    CssDependencies dependencies;
    dependencies.Add(One("button:not(.closed:hover *)"));

    EXPECT_TRUE(dependencies.classes_above);
    EXPECT_TRUE(dependencies.HasStateAbove("hover"));
  }
} // namespace
