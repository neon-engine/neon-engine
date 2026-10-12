#ifndef EDITOR_SCRIPT_RUNNER_HPP
#define EDITOR_SCRIPT_RUNNER_HPP

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>

#include <neon/logging/logger.hpp>
#include <neon/render/render-system.hpp>
#include <neon/tool-ui/tool-ui-context.hpp>

#include "editor-script.hpp"

namespace neon
{
  /// Runs an EditorScript on the tool interface, a step at a time, frame
  /// by frame. Before a frame it looks the label of the step up among the
  /// items the tools drew in the frame before and feeds the pointer and
  /// the keys that a hand would move, see ToolUiContext::FeedEvents(); a
  /// click is three frames, the pointer moved, the button down, and the
  /// button up, as the toolkit wants it. After a frame it checks what the
  /// step expects against what was drawn, saves the screenshot the step
  /// asks for, and moves on. It knows the tools through ToolUiContext
  /// alone, so it runs with a window and without one.
  ///
  /// A step that expects what is not so, or names a label that is not
  /// drawn within a few frames, fails: the error names the step and its
  /// line, and the run is to end with exit code 1. A script that ran to its
  /// end is done, and the run ends unless --frames says more.
  class EditorScriptRunner final
  {
    /// Frames a label may take to appear, as a menu's items do once the
    /// menu is open, before the step fails.
    static constexpr std::size_t frames_to_appear = 10;

    const EditorScript *_script;
    RenderSystem *_render_system;
    std::shared_ptr<Logger> _logger;

    std::size_t _index = 0;

    // where the step is: which part of a path, and which of the frames of
    // the click or the drag of that part
    std::size_t _part = 0;
    std::size_t _phase = 0;
    std::size_t _waited = 0;
    std::size_t _frames_looked = 0;

    // whether the step fed its last frame, and moves on after it
    bool _step_fed = false;

    bool _done = false;
    bool _failed = false;

    [[nodiscard]] const EditorScript::Step &Current() const;

    /// The step failed: the message is logged with the step and its line.
    void Fail(const std::string &message);

    /// Done with the step: the next one, or the end of the script.
    void Advance();

    /// The item the label names, of a kind that a click reaches, or
    /// nothing when it is not drawn yet. After frames_to_appear frames of
    /// nothing, the step fails.
    [[nodiscard]] std::optional<ToolUiItem> LookUp(
      const ToolUiContext &ui,
      const std::string &label,
      std::initializer_list<ToolUiItemKind> kinds);

    /// One frame of a click on the item: the pointer moved to it, the
    /// button down, the button up. Returns true in the frame of the up.
    bool Click(ToolUiContext &ui, const ToolUiPoint &center);

    /// Where a drag's end is, by its label in the items or as a place.
    [[nodiscard]] std::optional<ToolUiPoint> Place(const ToolUiContext &ui, const EditorScript::Target &target);

    void FeedMenu(ToolUiContext &ui);

    void FeedClick(ToolUiContext &ui);

    void FeedType(ToolUiContext &ui);

    void FeedKey(ToolUiContext &ui);

    void FeedPointer(ToolUiContext &ui);

    void FeedDrag(ToolUiContext &ui);

    void CheckWindow(const ToolUiContext &ui);

    void CheckText(const ToolUiContext &ui);

    void CheckItem(const ToolUiContext &ui);

    void TakeScreenshot();

  public:
    /// `render_system` saves the screenshots. The script outlives the
    /// runner.
    EditorScriptRunner(
      const EditorScript *script,
      RenderSystem *render_system,
      const std::shared_ptr<Logger> &logger);

    /// Feeds the events of the step for the frame that is begun next,
    /// found among the items of the frame before. Before BeginFrame().
    void BeforeFrame(ToolUiContext &ui);

    /// Checks the step against the frame that was drawn, saves what it
    /// asks for, and moves on. After the frame is finished.
    void AfterFrame(ToolUiContext &ui);

    /// Whether every step was done.
    [[nodiscard]] bool IsDone() const;

    /// Whether a step failed, which is said in the log.
    [[nodiscard]] bool HasFailed() const;

    /// Steps that are done so far, of all of them.
    [[nodiscard]] std::size_t GetStepsDone() const;

    [[nodiscard]] std::size_t GetStepCount() const;
  };
} // neon

#endif //EDITOR_SCRIPT_RUNNER_HPP
