#ifndef DATA_VALUE_HPP
#define DATA_VALUE_HPP

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace neon
{
  /// A value of a document, such as a scene file, after it was read and
  /// before it is turned into something the engine works with.
  ///
  /// It is one of six kinds. A list holds values in order. A map holds values
  /// under names, in the order they were written. Nothing in it says which
  /// format the document had, so what reads a scene does not depend on one.
  class DataValue
  {
  public:
    enum class Kind
    {
      /// Nothing was written.
      Empty = 0,
      Bool,
      Number,
      Text,
      List,
      Map
    };

    using Entry = std::pair<std::string, DataValue>;

  private:
    Kind _kind = Kind::Empty;
    bool _bool = false;
    double _number = 0.0;
    bool _single_precision = false;

    // whether the number was written as a double on purpose, with a d
    bool _precise = false;
    std::string _text;
    std::vector<DataValue> _items;
    std::vector<Entry> _entries;
    std::size_t _line = 0;

  public:
    DataValue() = default;

    static DataValue Bool(bool value);

    static DataValue Number(double value);

    /// A number that came from a `float`. It is written with the digits a
    /// `float` needs, so that 0.31f is written as 0.31.
    static DataValue Number(float value);

    static DataValue Number(int value);

    /// A number that a person wrote as a double on purpose, with the suffix
    /// `d`, as in `2.0d`. Reading it into a `float` is refused, since that
    /// would lose the precision that was asked for. The engine never writes
    /// the suffix: it knows the type of what it writes.
    static DataValue PreciseNumber(double value);

    static DataValue Text(const std::string &value);

    static DataValue List();

    static DataValue Map();

    [[nodiscard]] Kind GetKind() const;

    [[nodiscard]] bool IsEmpty() const;

    [[nodiscard]] bool IsList() const;

    [[nodiscard]] bool IsMap() const;

    /// The line of the document the value was read from, counted from 1.
    /// 0 when it is not known, which includes every value that was not read.
    [[nodiscard]] std::size_t GetLine() const;

    void SetLine(std::size_t line);

    /// Each returns false and leaves `value` alone when the value is of
    /// another kind.
    [[nodiscard]] bool GetBool(bool &value) const;

    [[nodiscard]] bool GetNumber(double &value) const;

    [[nodiscard]] bool GetNumber(float &value) const;

    [[nodiscard]] bool GetText(std::string &value) const;

    /// Whether the number came from a `float`.
    [[nodiscard]] bool IsSinglePrecision() const;

    /// Whether the number was written as a double on purpose, see
    /// PreciseNumber.
    [[nodiscard]] bool IsPrecise() const;

    /// The values of a list. Empty for every other kind.
    [[nodiscard]] const std::vector<DataValue> &GetItems() const;

    /// The values of a map with their names. Empty for every other kind.
    [[nodiscard]] const std::vector<Entry> &GetEntries() const;

    /// The value under a name of a map, or nullptr when there is none.
    [[nodiscard]] const DataValue *Find(const std::string &name) const;

    /// Adds to a list. Returns the value that was added.
    DataValue &Add(const DataValue &value);

    /// Sets the value under a name of a map, replacing the one that is there.
    /// Returns the value that was set.
    DataValue &Set(const std::string &name, const DataValue &value);

    /// What the kind is called in a message, such as `a list`.
    [[nodiscard]] static std::string Describe(Kind kind);
  };
} // neon

#endif //DATA_VALUE_HPP
