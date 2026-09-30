#ifndef CLIPBOARD_CONTEXT_HPP
#define CLIPBOARD_CONTEXT_HPP

#include <string>

namespace neon
{
  /// What is cut and copied, and what is pasted: the clipboard of the
  /// platform, as text in UTF-8. A backend implements it. Without one, a
  /// text that is copied stays inside the application, which
  /// Memory_Clipboard does.
  class ClipboardContext
  {
  protected:
    ~ClipboardContext() = default;

  public:
    /// Whether there is text to paste.
    [[nodiscard]] virtual bool HasText() = 0;

    /// The text, or empty when there is none.
    [[nodiscard]] virtual std::string GetText() = 0;

    /// Returns false when the platform did not take the text.
    virtual bool SetText(const std::string &text) = 0;
  };

  /// A clipboard that is kept in memory and shared with no other
  /// application. For a run without a window, and for tests.
  // ReSharper disable once CppInconsistentNaming
  class Memory_Clipboard final : public ClipboardContext
  {
    std::string _text;

  public:
    [[nodiscard]] bool HasText() override
    {
      return !_text.empty();
    }

    [[nodiscard]] std::string GetText() override
    {
      return _text;
    }

    bool SetText(const std::string &text) override
    {
      _text = text;
      return true;
    }
  };
} // neon

#endif //CLIPBOARD_CONTEXT_HPP
