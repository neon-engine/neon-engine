#ifndef RYML_DOCUMENT_FORMAT_HPP
#define RYML_DOCUMENT_FORMAT_HPP

#include <neon/data/document-format.hpp>

namespace neon
{
  /// Reads and writes YAML, with rapidyaml.
  ///
  /// What a value is follows the core schema of YAML. `true` and `false` are
  /// bools, `null`, `~`, and nothing are empty, what reads as a number is
  /// one, and everything else is text. A value in quotes is always text.
  ///
  /// Anchors, aliases, tags, and several documents in one text are refused.
  /// A scene does not need them, and they make a file harder to change by
  /// hand and by a tool.
  // ReSharper disable once CppInconsistentNaming
  class RYML_DocumentFormat final : public DocumentFormat
  {
  public:
    bool Read(
      const std::string &name,
      const std::string &text,
      DataValue &document,
      std::string &error) override;

    /// Lists of numbers are written on one line, such as `[0, 0, 2]`.
    /// Everything else is written as a block, with two spaces for a level.
    std::string Write(const DataValue &document) override;
  };
} // neon

#endif //RYML_DOCUMENT_FORMAT_HPP
