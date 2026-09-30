#ifndef DATA_READER_HPP
#define DATA_READER_HPP

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <neon/common/color.hpp>

#include "data-value.hpp"

namespace neon
{
  /// Reads the values of a map into variables, and collects what is wrong
  /// with them.
  ///
  /// A name that is not written leaves the variable as it is, so that what a
  /// document leaves out keeps its default. A name that is written and not
  /// asked for is reported by Finish(), which catches names that were
  /// misspelled.
  class DataReader
  {
    const DataValue *_map;
    std::string _document;
    std::string _where;
    std::vector<std::string> *_errors;

    // the names that were asked for, which are the names that are known
    mutable std::vector<std::string> _asked;

    [[nodiscard]] const DataValue *Ask(const std::string &name) const;

    [[nodiscard]] bool ReadNumbers(
      const std::string &name,
      float *numbers,
      std::size_t least,
      std::size_t most,
      std::size_t &count) const;

  public:
    /// `map` is what is read. `document` is what the document is called in
    /// messages, and `where` what is read from it, such as
    /// `Transform of entity 'bear'`. Both are for messages alone.
    DataReader(
      const DataValue &map,
      const std::string &document,
      const std::string &where,
      std::vector<std::string> &errors);

    [[nodiscard]] const std::string &GetWhere() const;

    /// Adds a message about a value, with the document and the line.
    void Report(const DataValue &value, const std::string &message) const;

    /// Whether the name is written.
    [[nodiscard]] bool Has(const std::string &name) const;

    /// Each returns whether the name was written and what it holds was read.
    /// What is written and of the wrong kind is reported.
    // ReSharper disable CppNonExplicitConvertingConstructor
    bool Read(const std::string &name, bool &value) const;

    bool Read(const std::string &name, float &value) const;

    /// A number without a fraction.
    bool Read(const std::string &name, int &value) const;

    bool Read(const std::string &name, std::string &value) const;

    bool Read(const std::string &name, std::vector<std::string> &value) const;

    /// A list of three numbers.
    bool Read(const std::string &name, glm::vec3 &value) const;

    /// A list of three numbers, or one number that stands for all three.
    bool ReadScale(const std::string &name, glm::vec3 &value) const;

    /// A list of three numbers for red, green, and blue, or four with alpha.
    bool Read(const std::string &name, Color &value) const;

    /// One of the given words. `value` becomes the index of the word.
    bool ReadChoice(const std::string &name, const std::vector<std::string> &choices, std::size_t &value) const;

    /// The map under a name, to be read in turn. `found` tells whether it
    /// was written. When it was not, the reader reads nothing.
    [[nodiscard]] DataReader ReadMap(const std::string &name, bool &found) const;

    /// The value under a name as it is, or nullptr.
    [[nodiscard]] const DataValue *ReadValue(const std::string &name) const;

    /// Reports every name that is written and was not asked for.
    void Finish() const;
  };
} // neon

#endif //DATA_READER_HPP
