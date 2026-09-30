#ifndef CSS_SELECTOR_HPP
#define CSS_SELECTOR_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// Selectors as https://www.w3.org/TR/selectors-4/ defines them: what they
// are written as, how much each counts, and which elements they match.

namespace neon
{
  /// An element as a selector sees it. What a selector is matched against
  /// implements this, so that selectors know nothing of the user interface
  /// they are used by.
  class CssElement
  {
  protected:
    ~CssElement() = default;

  public:
    /// What the element is, such as `button`. It is what a type selector
    /// names.
    [[nodiscard]] virtual const std::string &GetCssType() const = 0;

    /// What the element is called, which `#name` asks for. Empty for one
    /// without a name.
    [[nodiscard]] virtual const std::string &GetCssId() const = 0;

    [[nodiscard]] virtual bool HasCssClass(const std::string &name) const = 0;

    /// What an attribute holds, as text. Returns false for an element
    /// that does not have it.
    [[nodiscard]] virtual bool GetCssAttribute(const std::string &name, std::string &value) const = 0;

    /// Whether the element is in a state, named as its pseudo-class:
    /// `hover`, `active`, `focus`, `focus-within`, `disabled`, `checked`.
    [[nodiscard]] virtual bool IsInCssState(const std::string &name) const = 0;

    /// nullptr for the element at the top.
    [[nodiscard]] virtual const CssElement *GetCssParent() const = 0;

    /// The element in front of this one under the same parent, or nullptr.
    [[nodiscard]] virtual const CssElement *GetCssPreviousSibling() const = 0;

    /// The place among the elements under the same parent, counted from 1,
    /// and how many there are.
    [[nodiscard]] virtual std::size_t GetCssIndex() const = 0;

    [[nodiscard]] virtual std::size_t GetCssSiblingCount() const = 0;

    /// Whether the element has elements inside it.
    [[nodiscard]] virtual bool HasCssChildren() const = 0;
  };

  /// How much a selector counts when two rules write the same property:
  /// the identifiers, then the classes, attributes, and pseudo-classes,
  /// then the types and pseudo-elements.
  struct CssSpecificity
  {
    int ids = 0;
    int classes = 0;
    int types = 0;

    bool operator==(const CssSpecificity &other) const = default;

    [[nodiscard]] bool operator<(const CssSpecificity &other) const
    {
      if (ids != other.ids) { return ids < other.ids; }
      if (classes != other.classes) { return classes < other.classes; }
      return types < other.types;
    }
  };

  struct CssComplexSelector;

  /// A part of a compound selector: `.class`, `#id`, `[name=value]`,
  /// `:hover`, and so on.
  struct CssSimpleSelector
  {
    enum class Kind
    {
      Class = 0,
      Id,
      Attribute,
      PseudoClass
    };

    /// How the value of an attribute is compared.
    enum class Compare
    {
      /// `[name]`: the attribute is there.
      Exists = 0,
      /// `[name=value]`
      Equals,
      /// `[name~=value]`: one of the words is the value.
      Word,
      /// `[name|=value]`: the value, or the value and a hyphen in front.
      Dash,
      /// `[name^=value]`, `[name$=value]`, `[name*=value]`
      Prefix,
      Suffix,
      Contains
    };

    Kind kind = Kind::Class;

    /// The class, the identifier, the attribute, or the pseudo-class.
    std::string name;

    Compare compare = Compare::Exists;
    std::string value;
    bool ignores_case = false;

    /// `a` and `b` of `:nth-child(an+b)`.
    int step = 0;
    int offset = 0;

    /// What `:not()`, `:is()`, and `:where()` hold.
    std::vector<std::shared_ptr<CssComplexSelector>> arguments;
  };

  /// What stands between two compound selectors.
  enum class CssCombinator
  {
    /// Nothing in front: the first compound of a selector.
    None = 0,
    /// A space: somewhere inside.
    Descendant,
    /// `>`: directly inside.
    Child,
    /// `+`: directly behind, under the same parent.
    Adjacent,
    /// `~`: somewhere behind, under the same parent.
    Sibling
  };

  /// Selectors without a combinator between them, which all have to match
  /// one element: `button.primary:hover`.
  struct CssCompoundSelector
  {
    /// How this compound relates to the one in front of it.
    CssCombinator combinator = CssCombinator::None;

    /// The type, or empty for any, which includes `*`.
    std::string type;

    std::vector<CssSimpleSelector> parts;
  };

  /// A selector with its combinators: `panel.inventory > button:enabled`.
  struct CssComplexSelector
  {
    /// From left to right. The last one is what the selector is about.
    std::vector<CssCompoundSelector> compounds;

    /// The part of an element the selector is about, as `::name` says it.
    /// Empty for the element itself.
    std::string pseudo_element;

    CssSpecificity specificity;

    /// As it was written, with the spaces tidied.
    std::string text;
  };

  /// Reads a list of selectors that are set apart by commas. Returns false
  /// and says why when one of them cannot be read. As CSS says, a list with
  /// one selector that is wrong is wrong as a whole.
  [[nodiscard]] bool ParseCssSelectors(
    const std::string &text,
    std::vector<CssComplexSelector> &selectors,
    std::string &error);

  /// Whether the selector matches the element. A selector that is about a
  /// part of an element matches the element the part belongs to.
  [[nodiscard]] bool MatchesCss(const CssComplexSelector &selector, const CssElement &element);

  /// The same, where only elements inside `scope` are matched and `:scope`
  /// would be it. For a search that starts at an element.
  [[nodiscard]] bool MatchesCss(
    const CssComplexSelector &selector,
    const CssElement &element,
    const CssElement *scope);

  /// What in a selector depends on other elements than the one it is
  /// about. It says what has to be looked at again when an element
  /// changes.
  struct CssDependencies
  {
    /// States, classes, and attributes that are asked of an element above
    /// the one the selector is about.
    std::vector<std::string> states_above;
    bool classes_above = false;

    /// The same for an element in front of it.
    std::vector<std::string> states_before;
    bool classes_before = false;

    /// Whether the place among the siblings matters, as to `:first-child`.
    bool structure = false;

    void Add(const CssComplexSelector &selector);

    [[nodiscard]] bool HasStateAbove(const std::string &name) const;

    [[nodiscard]] bool HasStateBefore(const std::string &name) const;
  };
} // neon

#endif //CSS_SELECTOR_HPP
