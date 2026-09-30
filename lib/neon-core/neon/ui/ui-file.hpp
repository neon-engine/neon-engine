#ifndef UI_FILE_HPP
#define UI_FILE_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>

#include "ui-document.hpp"
#include "ui-element-types.hpp"

namespace neon
{
  /// Reads a user interface from a file.
  ///
  ///     ui: hud
  ///     version: 1
  ///
  ///     values:
  ///       health: 75
  ///
  ///     root:
  ///       type: panel
  ///       width: 100%
  ///       height: 100%
  ///       children:
  ///         - type: label
  ///           text: "Health: {health}"
  ///           position: absolute
  ///           top: 16
  ///           left: 16
  ///           font_size: 24
  ///
  /// What an element leaves out keeps its default. A name that is not
  /// known is an error, so that a name that was misspelled does not go
  /// unnoticed. Every problem of a file is found, not only the first.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed
  /// in. The file is read through the file system.
  class UiFile
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    const UiElementTypes *_types;

    struct Reading;

    std::unique_ptr<UiElement> ReadElement(
      const DataValue &value,
      const std::string &label,
      Reading &reading) const;

    void ReadStyles(const DataValue &value, const DataReader &reader, UiElement &element, Reading &reading) const;

    static void ReadTop(const DataReader &reader, UiDocument &document);

    void ReadStylesAndTemplates(const DataReader &reader, UiDocument &document, Reading &reading) const;

  public:
    /// The version of the layout of the file, and the highest that is
    /// read.
    static constexpr int version = 1;

    UiFile(FileSystemContext *file_system, DocumentFormat *format, const UiElementTypes *types);

    /// Reads the file at a virtual path. Returns nullptr when the file
    /// cannot be read or something in it is wrong, with a message for
    /// every problem in `errors`. Each names the file and the line.
    [[nodiscard]] std::unique_ptr<UiDocument> Read(const std::string &path, std::vector<std::string> &errors) const;

    /// Makes an element, with what is inside it, from what describes it:
    /// the values a file holds for an element. `document` is what the
    /// description is called in messages. Returns nullptr when something in
    /// it is wrong, with a message for every problem in `errors`.
    [[nodiscard]] std::unique_ptr<UiElement> CreateElement(
      const DataValue &description,
      const std::string &document,
      std::vector<std::string> &errors) const;

    /// The same from text in the format of the files, such as
    /// `{type: label, text: Hello}`.
    [[nodiscard]] std::unique_ptr<UiElement> CreateElementFromText(
      const std::string &text,
      const std::string &document,
      std::vector<std::string> &errors) const;

    /// Reads the style sheets a document names again, in place of those it
    /// holds. Returns false and leaves the document as it is when a sheet
    /// cannot be read.
    bool ReloadStyles(UiDocument &document, std::vector<std::string> &errors) const;
  };
} // neon

#endif //UI_FILE_HPP
