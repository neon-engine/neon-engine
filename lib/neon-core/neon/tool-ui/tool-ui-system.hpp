#ifndef TOOL_UI_SYSTEM_HPP
#define TOOL_UI_SYSTEM_HPP

#include <memory>

#include "tool-ui-context.hpp"
#include "neon/logging/logger.hpp"

namespace neon
{
  /// The base class of the backends of the tool interface: what the editor
  /// draws its windows with, see ToolUiContext, with its lifecycle. A
  /// backend draws with the window and the renderer of the application,
  /// so it comes up after both and goes before them.
  class ToolUiSystem : public ToolUiContext
  {
  protected:
    std::shared_ptr<Logger> _logger;

    ~ToolUiSystem() = default;

  public:
    explicit ToolUiSystem(const std::shared_ptr<Logger> &logger)
    {
      _logger = logger;
    }

    /// Brings the backend up, once the window and the renderer are up.
    /// Returns false when it cannot draw with them, which is said.
    virtual bool Initialize() = 0;

    /// Takes the backend down, before the renderer and the window go.
    /// Safe to call more than once.
    virtual void CleanUp() = 0;

    /// Starts the frame of the tool interface, once the input of the
    /// frame is read and the renderer has begun its frame. What is
    /// declared from here on is shown.
    virtual void BeginFrame() = 0;

    /// Draws what was declared since BeginFrame() into the frame of the
    /// renderer, on top of everything else drawn into it, before the
    /// renderer finishes the frame.
    virtual void EndFrame() = 0;
  };
} // neon

#endif //TOOL_UI_SYSTEM_HPP
