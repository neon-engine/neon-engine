#include "editor-log.hpp"

namespace neon
{
  EditorLog::EditorLog(LoggingSystem *logging_system)
  {
    _logging_system = logging_system;
  }

  void EditorLog::Draw(ToolUiContext &ui)
  {
    if (ui.BeginWindow(std::string(title)))
    {
      // copied only when a line was added since, which is most frames at
      // the start and few afterwards
      if (const std::uint64_t revision = _logging_system->GetLineRevision(); revision != _revision)
      {
        _revision = revision;
        _logging_system->CopyRecentLines(_lines);
        _changed = true;
      }

      for (const std::string &line : _lines) { ui.Text(line); }

      if (_changed)
      {
        ui.ScrollToBottom();
        _changed = false;
      }
    }
    ui.EndWindow();
  }
} // neon
