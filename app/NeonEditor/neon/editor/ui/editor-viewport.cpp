#include "editor-viewport.hpp"

#include <algorithm>
#include <cmath>

#include <neon/input/input-state.hpp>
#include <neon/render/render-target-options.hpp>

namespace neon
{
  EditorViewport::EditorViewport(
    RenderContext *render_context,
    RenderPipeline *render_pipeline,
    WindowContext *window_context,
    InputContext *input_context,
    EditorCamera *camera,
    const std::shared_ptr<Logger> &logger)
  {
    _render_context = render_context;
    _render_pipeline = render_pipeline;
    _window_context = window_context;
    _input_context = input_context;
    _camera = camera;
    _logger = logger;
  }

  EditorCameraInput EditorViewport::ReadInput(ToolUiContext &ui)
  {
    const InputState &state = _input_context->GetInputState();
    EditorCameraInput input;

    // The view is held while the right button is: taken over the window,
    // kept wherever the pointer goes, and released with the button, with
    // the pointer shown again where it was.
    const bool right_down = state.IsMouseButtonDown(MouseButton::Right);
    if (!_looking && right_down && ui.IsWindowHovered())
    {
      _looking = true;
      _input_context->CenterAndHideCursor();
    } else if (_looking && !right_down)
    {
      _looking = false;
      _input_context->ShowCursor();
    }

    // by how far the mouse went in this frame, which is nothing while it
    // rests; the axis of the map keeps the last motion and is the game's
    if (_looking)
    {
      const AxisState &motion = state.GetMouseMotion();
      input.look_x = static_cast<float>(motion.x);
      input.look_y = static_cast<float>(motion.y);
    }

    // the keys, while the pointer is over the window or it has the focus,
    // and no field of the tools is typed into
    const bool takes_keys = (_looking || ui.IsWindowHovered() || ui.IsWindowFocused()) && !ui.WantsKeyboard();
    if (takes_keys)
    {
      const auto axis = [&state](const Key positive, const Key negative)
      {
        return (state.IsKeyDown(positive) ? 1.0f : 0.0f) - (state.IsKeyDown(negative) ? 1.0f : 0.0f);
      };
      input.move_forward = axis(Key::W, Key::S);
      input.move_right = axis(Key::D, Key::A);
      input.move_up = (state.IsKeyDown(Key::E) || state.IsKeyDown(Key::Space) ? 1.0f : 0.0f) -
                      (state.IsKeyDown(Key::Q) ? 1.0f : 0.0f);
      input.fast = state.IsKeyDown(Key::LeftShift) || state.IsKeyDown(Key::RightShift);
    }

    // the wheel, over the window: up is faster
    if (_looking || ui.IsWindowHovered()) { input.speed_notches = -static_cast<float>(state.GetWheel().y); }

    return input;
  }

  WindowSize EditorViewport::PixelsOf(const ToolUiSize &room, const WindowMetrics &metrics)
  {
    const int width = static_cast<int>(std::lround(metrics.ToPixelsX(room.width)));
    const int height = static_cast<int>(std::lround(metrics.ToPixelsY(room.height)));
    return {std::max(width, 1), std::max(height, 1)};
  }

  void EditorViewport::Prepare()
  {
    // no room yet, or a window that is collapsed
    if (_room.width < 1.0f || _room.height < 1.0f)
    {
      _render_pipeline->SetFrameTarget(No_Render_Target);
      return;
    }

    const auto [width, height] = PixelsOf(_room, _window_context->GetMetrics());
    if (_target != No_Render_Target && (width != _width || height != _height))
    {
      _render_context->DestroyRenderTarget(_target);
      _target = No_Render_Target;
    }

    if (_target == No_Render_Target)
    {
      // one level: the image is shown at its size, and never scaled by
      // the quality of the game
      const RenderTargetOptions options{.mipmaps = 1, .scales = false};
      _target = _render_context->CreateRenderTarget(std::string(name), width, height, options);
      _width = width;
      _height = height;

      if (_target == No_Render_Target)
      {
        _logger->Error("The viewport cannot be made at {} by {} pixels, the scene is not shown", width, height);
      }
    }

    _render_pipeline->SetFrameTarget(_target);
  }

  void EditorViewport::Draw(ToolUiContext &ui)
  {
    if (ui.BeginWindow(std::string(title), false))
    {
      const ToolUiSize room = ui.GetAvailableSize();
      if (_target != No_Render_Target) { ui.ImageOfRenderTarget(_target, room.width, room.height); }
      _room = room;

      _camera->Update(ReadInput(ui), _window_context->GetDeltaTime());
    } else
    {
      _room = {};
      if (_looking)
      {
        _looking = false;
        _input_context->ShowCursor();
      }
    }
    ui.EndWindow();
  }

  void EditorViewport::CleanUp()
  {
    if (_looking)
    {
      _looking = false;
      _input_context->ShowCursor();
    }
    _render_pipeline->SetFrameTarget(No_Render_Target);
    if (_target != No_Render_Target)
    {
      _render_context->DestroyRenderTarget(_target);
      _target = No_Render_Target;
    }
    _room = {};
  }
} // neon
