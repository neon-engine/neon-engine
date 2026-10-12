#ifndef EDITOR_VIEWPORT_HPP
#define EDITOR_VIEWPORT_HPP

#include <memory>
#include <string>
#include <string_view>

#include <neon/input/input-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/editor/camera/editor-camera.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/render/render-context.hpp>
#include <neon/render/render-pipeline.hpp>
#include <neon/tool-ui/tool-ui-context.hpp>
#include <neon/window/window-context.hpp>

namespace neon
{
  /// The window of the editor that shows the scene: a render target the
  /// camera of the window draws into in place of the frame, shown as an
  /// image at the size of the window. The target has the size the window
  /// had in the frame before, in pixels of the display, and is made again
  /// when the window changes its size.
  ///
  /// The editor camera flies by the devices while the pointer is over the
  /// window or it has the focus: the right mouse button held turns the
  /// view, with the pointer hidden and held until the button is released;
  /// W, A, S, and D move, E and space go up, Q goes down, shift is faster,
  /// and the wheel changes the speed. Keys are read only while no field of
  /// the tools takes them. The devices are read as they are, not the
  /// actions of the game's input map, which are the game's.
  class EditorViewport final
  {
    RenderContext *_render_context;
    RenderPipeline *_render_pipeline;
    WindowContext *_window_context;
    InputContext *_input_context;
    EditorCamera *_camera;
    std::shared_ptr<Logger> _logger;

    // whether the right button holds the view, with the pointer hidden
    bool _looking = false;

    /// What the devices say for the camera in this frame, and takes and
    /// releases the pointer as the right button goes down and up.
    [[nodiscard]] EditorCameraInput ReadInput(ToolUiContext &ui);

    // the target, and its size in pixels
    int _target = No_Render_Target;
    int _width = 0;
    int _height = 0;

    // the room the window had for the image in the frame before, in points
    ToolUiSize _room;

  public:
    /// What the target is known as, and the title of the window.
    static constexpr std::string_view name = "editor-viewport";
    static constexpr std::string_view title = "Viewport";

    EditorViewport(
      RenderContext *render_context,
      RenderPipeline *render_pipeline,
      WindowContext *window_context,
      InputContext *input_context,
      EditorCamera *camera,
      const std::shared_ptr<Logger> &logger);

    /// Has the target be the size of the room the window had, made again
    /// when that changed, and tells the pipeline to draw into it. Called
    /// before the world draws. Without room, as before the first frame,
    /// the pipeline draws into the frame.
    void Prepare();

    /// Declares the window with the target in it, measures the room for
    /// the next frame, and flies the camera by the devices.
    void Draw(ToolUiContext &ui);

    /// Whether the right button holds the view.
    [[nodiscard]] bool IsLooking() const { return _looking; }

    /// Frees the target, and has the pipeline draw into the frame again.
    void CleanUp();

    /// The target, or No_Render_Target before the first frame with room.
    [[nodiscard]] int GetTarget() const { return _target; }

    /// The size the target has to be for a room of `room` points on a
    /// window of `metrics`: whole pixels, and none below 1 by 1.
    [[nodiscard]] static WindowSize PixelsOf(const ToolUiSize &room, const WindowMetrics &metrics);
  };
} // neon

#endif //EDITOR_VIEWPORT_HPP
