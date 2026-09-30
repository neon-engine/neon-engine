#ifndef DOCUMENT_FORMAT_HPP
#define DOCUMENT_FORMAT_HPP

#include <string>

#include "data-value.hpp"

namespace neon
{
  /// Turns the text of a document into values and back. An implementation
  /// knows one format, such as YAML. What it is given and what it gives back
  /// is text, so it reads and writes no files. Those go through the file
  /// system.
  class DocumentFormat
  {
  protected:
    ~DocumentFormat() = default;

  public:
    /// Reads a document. `name` is what the document is called in messages,
    /// such as its virtual path. Returns false when the text is not a valid
    /// document, and says why in `error`.
    virtual bool Read(
      const std::string &name,
      const std::string &text,
      DataValue &document,
      std::string &error) = 0;

    /// Writes a document in a form that is meant to be read and changed by
    /// hand.
    virtual std::string Write(const DataValue &document) = 0;
  };
} // neon

#endif //DOCUMENT_FORMAT_HPP
