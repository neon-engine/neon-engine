#ifndef FIELD_DOCUMENTS_HPP
#define FIELD_DOCUMENTS_HPP

#include <neon/data/data-reader.hpp>
#include <neon/data/data-value.hpp>

#include "type-info.hpp"

namespace neon
{
  /// Reads the fields of an object from a map of a document, as its
  /// description says. `object` points to an object of the described type.
  ///
  /// A field that is not written keeps the value the object has. What is
  /// written and cannot be taken is reported through the reader, and so is a
  /// field that is required and has no value afterwards.
  void ReadFields(const TypeInfo &type, const DataReader &reader, void *object);

  /// Writes the fields of an object into a map. A field that holds the same
  /// as it does in `standard` is left out, unless it is always written.
  /// `standard` is an object with the defaults of the type.
  void WriteFields(const TypeInfo &type, const void *object, const void *standard, DataValue &map);

  /// A value of a field as a value of a document.
  [[nodiscard]] DataValue ToDataValue(const FieldValue &value);
} // neon

#endif //FIELD_DOCUMENTS_HPP
