#ifndef TOOL_UI_ITEM_KIND_HPP
#define TOOL_UI_ITEM_KIND_HPP

#include <string>

namespace neon
{
  /// What kind of thing an item of the tool interface is, see ToolUiItem.
  enum class ToolUiItemKind
  {
    /// A window that is shown, by its title. Collapsed, it is shown still.
    Window = 0,

    /// A menu of the bar or of another menu, by its label, whether or not
    /// it is open.
    Menu,

    /// An item of a menu that is open.
    MenuItem,
    Button,
    Checkbox,

    /// A field of text, by its label.
    InputText,

    /// A text that was drawn, by the text itself.
    Text
  };

  /// What a kind is called in a script and in a message, such as `menu-item`.
  [[nodiscard]] inline std::string NameOf(const ToolUiItemKind kind)
  {
    switch (kind)
    {
      case ToolUiItemKind::Window: return "window";
      case ToolUiItemKind::Menu: return "menu";
      case ToolUiItemKind::MenuItem: return "menu-item";
      case ToolUiItemKind::Button: return "button";
      case ToolUiItemKind::Checkbox: return "checkbox";
      case ToolUiItemKind::InputText: return "input-text";
      case ToolUiItemKind::Text: return "text";
    }
    return "unknown";
  }
} // neon

#endif //TOOL_UI_ITEM_KIND_HPP
