#include "editor-script-runner.hpp"

#include <algorithm>
#include <format>

namespace neon
{
  // Helpers of EditorScriptRunner, for this file alone.
  namespace
  {
    /// Frames the pointer is moved over between the ends of a drag, so
    /// that the toolkit sees it travel.
    constexpr std::size_t drag_moves = 3;

    /// What a click reaches.
    constexpr std::initializer_list<ToolUiItemKind> clickable = {
      ToolUiItemKind::Button, ToolUiItemKind::MenuItem, ToolUiItemKind::Checkbox, ToolUiItemKind::InputText
    };
  }

  EditorScriptRunner::EditorScriptRunner(
    const EditorScript *script,
    RenderSystem *render_system,
    const std::shared_ptr<Logger> &logger)
  {
    _script = script;
    _render_system = render_system;
    _logger = logger;
    _done = script->IsEmpty();
  }

  const EditorScript::Step &EditorScriptRunner::Current() const
  {
    return _script->GetSteps()[_index];
  }

  void EditorScriptRunner::Fail(const std::string &message)
  {
    const EditorScript::Step &step = Current();
    const std::size_t number = _index + 1;
    const std::string described = step.Describe();
    _logger->Error("Step {} of the script, '{}' on line {}, failed: {}", number, described, step.line, message);
    _failed = true;
  }

  void EditorScriptRunner::Advance()
  {
    const std::size_t number = _index + 1;
    const std::string described = Current().Describe();
    _logger->Info("Step {} of the script, '{}' on line {}, is done", number, described, Current().line);
    _index++;
    _part = 0;
    _phase = 0;
    _waited = 0;
    _frames_looked = 0;
    _step_fed = false;
    if (_index >= _script->GetSteps().size())
    {
      _done = true;
      const std::size_t count = _script->GetSteps().size();
      _logger->Info("The script is done, all {} steps of it", count);
    }
  }

  std::optional<ToolUiItem> EditorScriptRunner::LookUp(
    const ToolUiContext &ui,
    const std::string &label,
    const std::initializer_list<ToolUiItemKind> kinds)
  {
    for (const ToolUiItemKind kind : kinds)
    {
      if (std::optional<ToolUiItem> item = ui.FindItem(label, kind); item.has_value())
      {
        _frames_looked = 0;
        return item;
      }
    }

    _frames_looked++;
    if (_frames_looked > frames_to_appear)
    {
      std::string drawn;
      for (const ToolUiItem &item : ui.GetItemsDrawn())
      {
        if (item.kind == ToolUiItemKind::Text || std::ranges::find(kinds, item.kind) == kinds.end()) { continue; }
        if (!drawn.empty()) { drawn += ", "; }
        drawn += NameOf(item.kind) + " '" + item.label + "'";
      }
      Fail(std::format("'{}' was not drawn in {} frames. Drawn: {}", label, frames_to_appear, drawn.empty() ? "nothing of that kind" : drawn));
    }
    return std::nullopt;
  }

  bool EditorScriptRunner::Click(ToolUiContext &ui, const ToolUiPoint &center)
  {
    ToolUiEvents events;
    switch (_phase)
    {
      case 0: events.pointer = center;
        break;
      case 1: events.pointer = center;
        events.button_down = true;
        break;
      default: events.pointer = center;
        events.button_down = false;
        break;
    }
    ui.FeedEvents(events);

    if (_phase < 2)
    {
      _phase++;
      return false;
    }
    _phase = 0;
    return true;
  }

  std::optional<ToolUiPoint> EditorScriptRunner::Place(const ToolUiContext &ui, const EditorScript::Target &target)
  {
    if (target.is_point) { return target.point; }
    const std::optional<ToolUiItem> item = LookUp(ui, target.label, {
      ToolUiItemKind::Window, ToolUiItemKind::Button, ToolUiItemKind::MenuItem, ToolUiItemKind::Checkbox,
      ToolUiItemKind::InputText, ToolUiItemKind::Menu, ToolUiItemKind::Text
    });
    if (!item.has_value()) { return std::nullopt; }
    return item->rect.Center();
  }

  void EditorScriptRunner::FeedMenu(ToolUiContext &ui)
  {
    const EditorScript::Step &step = Current();
    const bool last = _part + 1 == step.path.size();

    // the menus of the path are menus; the last part is the item chosen,
    // or a menu when the path ends on one
    const std::optional<ToolUiItem> item = last
      ? LookUp(ui, step.path[_part], {ToolUiItemKind::MenuItem, ToolUiItemKind::Menu})
      : LookUp(ui, step.path[_part], {ToolUiItemKind::Menu});
    if (!item.has_value()) { return; }

    if (!Click(ui, item->rect.Center())) { return; }
    if (last) { _step_fed = true; }
    else { _part++; }
  }

  void EditorScriptRunner::FeedClick(ToolUiContext &ui)
  {
    const std::optional<ToolUiItem> item = LookUp(ui, Current().label, clickable);
    if (item.has_value() && Click(ui, item->rect.Center())) { _step_fed = true; }
  }

  void EditorScriptRunner::FeedType(ToolUiContext &ui)
  {
    // the field is clicked first, so that it has the focus, and the text
    // goes in the frame after
    if (_part == 0)
    {
      const std::optional<ToolUiItem> item = LookUp(ui, Current().label, {ToolUiItemKind::InputText});
      if (item.has_value() && Click(ui, item->rect.Center())) { _part = 1; }
      return;
    }

    ToolUiEvents events;
    events.text = Current().text;
    ui.FeedEvents(events);
    _step_fed = true;
  }

  void EditorScriptRunner::FeedKey(ToolUiContext &ui)
  {
    ToolUiEvents events;
    KeyEvent down = Current().key;
    down.is_down = true;
    KeyEvent up = Current().key;
    up.is_down = false;
    events.keys = {down, up};
    ui.FeedEvents(events);
    _step_fed = true;
  }

  void EditorScriptRunner::FeedPointer(ToolUiContext &ui)
  {
    ToolUiEvents events;
    events.pointer = Current().point;
    ui.FeedEvents(events);
    _step_fed = true;
  }

  void EditorScriptRunner::FeedDrag(ToolUiContext &ui)
  {
    const std::optional<ToolUiPoint> from = Place(ui, Current().from);
    if (!from.has_value()) { return; }
    const std::optional<ToolUiPoint> to = Place(ui, Current().to);
    if (!to.has_value()) { return; }

    // the pointer at the start, the button down, the pointer on its way
    // in steps, and the button up at the end
    ToolUiEvents events;
    if (_phase == 0) { events.pointer = from; }
    else if (_phase == 1)
    {
      events.pointer = from;
      events.button_down = true;
    } else if (_phase <= 1 + drag_moves)
    {
      const float share = static_cast<float>(_phase - 1) / static_cast<float>(drag_moves);
      events.pointer = ToolUiPoint{from->x + (to->x - from->x) * share, from->y + (to->y - from->y) * share};
    } else
    {
      events.pointer = to;
      events.button_down = false;
      _step_fed = true;
    }
    ui.FeedEvents(events);
    _phase++;
  }

  void EditorScriptRunner::BeforeFrame(ToolUiContext &ui)
  {
    if (_done || _failed) { return; }

    switch (Current().kind)
    {
      case EditorScript::Kind::Menu: FeedMenu(ui);
        break;
      case EditorScript::Kind::Click: FeedClick(ui);
        break;
      case EditorScript::Kind::Type: FeedType(ui);
        break;
      case EditorScript::Kind::Key: FeedKey(ui);
        break;
      case EditorScript::Kind::Pointer: FeedPointer(ui);
        break;
      case EditorScript::Kind::Drag: FeedDrag(ui);
        break;
      default: break;
    }
  }

  void EditorScriptRunner::CheckWindow(const ToolUiContext &ui)
  {
    const EditorScript::Step &step = Current();
    const bool shown = ui.FindItem(step.label, ToolUiItemKind::Window).has_value();
    if (shown == step.expected)
    {
      Advance();
      return;
    }

    std::string windows;
    for (const ToolUiItem &item : ui.GetItemsDrawn())
    {
      if (item.kind != ToolUiItemKind::Window) { continue; }
      if (!windows.empty()) { windows += ", "; }
      windows += "'" + item.label + "'";
    }
    Fail(std::format(
      "the window '{}' is {}. Shown: {}", step.label, shown ? "shown" : "not shown", windows.empty() ? "no window" : windows));
  }

  void EditorScriptRunner::CheckText(const ToolUiContext &ui)
  {
    const EditorScript::Step &step = Current();
    const bool drawn = std::ranges::any_of(ui.GetItemsDrawn(), [&](const ToolUiItem &item)
    {
      return item.kind == ToolUiItemKind::Text && item.label.find(step.text) != std::string::npos;
    });
    if (drawn) { Advance(); }
    else { Fail(std::format("no text drawn in this frame holds '{}'", step.text)); }
  }

  void EditorScriptRunner::CheckItem(const ToolUiContext &ui)
  {
    const EditorScript::Step &step = Current();
    const bool present = std::ranges::any_of(ui.GetItemsDrawn(), [&](const ToolUiItem &item)
    {
      return item.kind != ToolUiItemKind::Text && item.label == step.label;
    });
    if (present == step.expected) { Advance(); }
    else { Fail(std::format("'{}' is {} in this frame", step.label, present ? "drawn" : "not drawn")); }
  }

  void EditorScriptRunner::TakeScreenshot()
  {
    const std::string &path = Current().label;
    if (!_render_system->CaptureFrame(path))
    {
      Fail(std::format("the frame could not be saved to {}, see above", path));
      return;
    }
    _logger->Info("The script saved the frame to {}", path);
    Advance();
  }

  void EditorScriptRunner::AfterFrame(ToolUiContext &ui)
  {
    if (_done || _failed) { return; }

    switch (Current().kind)
    {
      case EditorScript::Kind::Wait:
        _waited++;
        if (_waited >= Current().frames) { Advance(); }
        break;
      case EditorScript::Kind::Screenshot: TakeScreenshot();
        break;
      case EditorScript::Kind::ExpectWindow: CheckWindow(ui);
        break;
      case EditorScript::Kind::ExpectText: CheckText(ui);
        break;
      case EditorScript::Kind::ExpectItem: CheckItem(ui);
        break;
      default:
        if (_step_fed) { Advance(); }
        break;
    }
  }

  bool EditorScriptRunner::IsDone() const
  {
    return _done;
  }

  bool EditorScriptRunner::HasFailed() const
  {
    return _failed;
  }

  std::size_t EditorScriptRunner::GetStepsDone() const
  {
    return _index;
  }

  std::size_t EditorScriptRunner::GetStepCount() const
  {
    return _script->GetSteps().size();
  }
} // neon
