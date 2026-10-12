#ifndef EDITOR_SCRIPT_HPP
#define EDITOR_SCRIPT_HPP

#include <cstddef>
#include <string>
#include <vector>

#include <neon/data/data-value.hpp>
#include <neon/input/key.hpp>
#include <neon/tool-ui/tool-ui-point.hpp>

namespace neon
{
  /// What a hand would do in the editor, written down, so that a run
  /// without a window proves a tool window as a checklist would, see
  /// docs/editor.md#scripted-runs. A script is a `*.editor-script.yml`
  /// file, a map with `steps`, each step a map of one key:
  ///
  ///     description: View > Log shows and hides the Log
  ///     steps:
  ///       - menu: View > Log
  ///       - wait: 2
  ///       - expect: {window: Log, shown: true}
  ///       - screenshot: output://log-shown.png
  ///       - menu: View > Log
  ///       - expect: {window: Log, shown: false}
  ///
  /// The steps speak of the tools by their labels, as a checklist does,
  /// and never of the toolkit: the runner finds each label among the
  /// items the tool interface drew in the frame before, see ToolUiItem,
  /// and feeds the pointer and the keys through ToolUiContext::FeedEvents().
  ///
  /// | Step | Does |
  /// |---|---|
  /// | `menu: File > Quit` | Opens each menu of the path in turn and chooses the last item |
  /// | `click: Close` | Presses a button, a menu item that is shown, or a checkbox, by its label |
  /// | `type: {field: Name, text: my-game}` | Clicks a text field by its label and types into it |
  /// | `key: Ctrl+L` | Presses a key with what is held with it: Ctrl, Shift, Alt, Cmd, or Shortcut for the platform's |
  /// | `pointer: {x: 200, y: 100}` | Puts the pointer there, in points of the window |
  /// | `drag: {from: Log, to: "900,300"}` | Holds the button down over a label or a place and lets it go over another |
  /// | `wait: 2` | Lets that many frames pass |
  /// | `screenshot: output://name.png` | Saves the frame, with the tools in it |
  /// | `expect: {window: Log, shown: true}` | Whether a window of that title is shown in this frame |
  /// | `expect: {text: "Quit was chosen"}` | Whether a text drawn in this frame holds the text |
  /// | `expect: {item: Close, present: true}` | Whether anything of that label was drawn in this frame |
  class EditorScript final
  {
  public:
    enum class Kind
    {
      Menu = 0,
      Click,
      Type,
      Key,
      Pointer,
      Drag,
      Wait,
      Screenshot,
      ExpectWindow,
      ExpectText,
      ExpectItem
    };

    /// Where a drag starts or ends: something by its label, or a place.
    struct Target
    {
      std::string label;
      ToolUiPoint point;
      bool is_point = false;

      bool operator==(const Target &other) const = default;
    };

    struct Step
    {
      Kind kind = Kind::Wait;

      /// The line of the script the step is on, for messages.
      std::size_t line = 0;

      /// The menus of a path, the item chosen last.
      std::vector<std::string> path;

      /// The label of what is clicked, typed into, or expected; the title
      /// of a window; the path of a screenshot.
      std::string label;

      /// What is typed, or the text expected.
      std::string text;

      /// The key pressed, with what is held with it.
      KeyEvent key;

      /// Where the pointer goes.
      ToolUiPoint point;

      Target from;
      Target to;

      /// Frames a wait lasts.
      std::size_t frames = 1;

      /// Whether the window is to be shown, or the item to be present.
      bool expected = true;

      /// The step as the script wrote it, for messages: `menu View > Log`.
      [[nodiscard]] std::string Describe() const;
    };

  private:
    std::string _description;
    std::vector<Step> _steps;

  public:
    /// Reads a script that was read as a document. `name` is what the
    /// script is called in messages. Returns false with a message for
    /// everything that is wrong in `errors`, each naming the script and
    /// the line; the script is empty then.
    static bool Read(
      const DataValue &document,
      const std::string &name,
      EditorScript &script,
      std::vector<std::string> &errors);

    /// A chord as a script writes it, such as `Ctrl+Shift+L` or `enter`:
    /// the key last, what is held with it in front, in any case. Returns
    /// false for a part that is no key and no modifier.
    static bool ReadChord(const std::string &text, KeyEvent &key);

    [[nodiscard]] const std::string &GetDescription() const;

    [[nodiscard]] const std::vector<Step> &GetSteps() const;

    [[nodiscard]] bool IsEmpty() const;
  };
} // neon

#endif //EDITOR_SCRIPT_HPP
