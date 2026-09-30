#ifndef UI_VALUES_HPP
#define UI_VALUES_HPP

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <neon/data/data-value.hpp>

namespace neon
{
  /// A value a game hands to its user interface: a number, a text, or a
  /// flag.
  struct UiValue
  {
    enum class Kind
    {
      Number = 0,
      Text,
      Flag
    };

    Kind kind = Kind::Number;
    double number = 0.0;
    std::string text;
    bool flag = false;

    static UiValue Number(double number);

    static UiValue Text(const std::string &text);

    static UiValue Flag(bool flag);

    /// As it is shown. A whole number is written without a point, any
    /// other with two digits behind it. `decimals` of 0 and above asks for
    /// that many digits. A flag is `true` or `false`.
    [[nodiscard]] std::string AsText(int decimals = -1) const;

    /// A text counts as the number it holds, and as `otherwise` when it
    /// holds none. A flag counts as 1 or 0.
    [[nodiscard]] double AsNumber(double otherwise) const;

    /// A number counts as true unless it is 0, a text unless it is empty.
    [[nodiscard]] bool AsFlag() const;

    bool operator==(const UiValue &other) const = default;
  };

  /// The values of a game, by name. A user interface refers to them as
  /// `{health}`, and shows what they hold at the time it is drawn.
  class UiValues
  {
    std::map<std::string, UiValue> _values;
    std::uint64_t _revision = 0;

    // the names that were asked for and are not there, each of them once
    mutable std::set<std::string> _missed;
    mutable std::vector<std::string> _newly_missed;

  public:
    /// Sets a value. Setting it to what it already holds changes nothing.
    void Set(const std::string &name, const UiValue &value);

    /// Sets a value unless there is one of that name. For the values a
    /// file starts with, which must not replace what the game has set.
    void SetDefault(const std::string &name, const UiValue &value);

    /// The value, or nullptr when there is none of that name.
    [[nodiscard]] const UiValue *Find(const std::string &name) const;

    /// Goes up whenever a value changes, so that what was made from the
    /// values can tell whether it is still right.
    [[nodiscard]] std::uint64_t GetRevision() const;

    /// The names that were asked for in vain since the last call. Each is
    /// handed out once, so that it is reported once and not in every frame.
    [[nodiscard]] std::vector<std::string> TakeMissed() const;

    /// Where a value is looked for that is not among these. The values of
    /// one user interface fall back on those every user interface shares.
    void SetParent(const UiValues *parent);

    /// Whether there is a value of that name, here or where these fall
    /// back on. Asking is not counted as asking in vain.
    [[nodiscard]] bool Has(const std::string &name) const;

  private:
    const UiValues *_parent = nullptr;
  };

  /// A text with places for values in it, such as `Health: {health}`.
  ///
  ///     {name}      the value as it is shown
  ///     {name:1}    a number with one digit behind the point
  ///     {{ and }}   the brackets themselves
  ///
  /// A name that has no value is shown as it is written, brackets
  /// included, so that a name that was misspelled is seen on the screen.
  class UiTemplate
  {
    struct Part
    {
      bool is_value = false;
      // the text, or the name of the value
      std::string text;
      int decimals = -1;
    };

    std::vector<Part> _parts;

  public:
    /// Returns false and says why when a bracket is left open or what is
    /// between brackets is no name.
    static bool Parse(const std::string &text, UiTemplate &result, std::string &error);

    [[nodiscard]] std::string Format(const UiValues &values) const;

    /// Whether the text changes with the values.
    [[nodiscard]] bool HasValues() const;
  };

  /// A number that is written, or that follows a value: `75` or
  /// `"{health}"`.
  class UiNumber
  {
    double _number = 0.0;
    std::string _name;

  public:
    UiNumber() = default;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    UiNumber(double number);

    /// Returns false when the value is neither a number nor the name of a
    /// value in brackets.
    static bool Read(const DataValue &value, UiNumber &result);

    /// What the number is now. A name that has no value counts as
    /// `otherwise`.
    [[nodiscard]] double Get(const UiValues &values, double otherwise) const;
  };

  /// A flag that is written, or that follows a value: `true`,
  /// `"{paused}"`, or `"{!paused}"` for the opposite.
  class UiFlag
  {
    bool _flag = false;
    bool _is_opposite = false;
    std::string _name;

  public:
    UiFlag() = default;

    // ReSharper disable once CppNonExplicitConvertingConstructor
    UiFlag(bool flag);

    static bool Read(const DataValue &value, UiFlag &result);

    /// What the flag is now. A name that has no value counts as
    /// `otherwise`, whether the opposite is asked for or not.
    [[nodiscard]] bool Get(const UiValues &values, bool otherwise) const;
  };
} // neon

#endif //UI_VALUES_HPP
