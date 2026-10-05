#ifndef WINDOW_MODE_HPP
#define WINDOW_MODE_HPP

/// How the window is shown. Set at the start by `window.mode` of the
/// settings, and changed while the application runs with
/// WindowContext::SetWindowMode().
enum class WindowMode
{
  /// A regular window with a title bar and borders, sized by width and height.
  Windowed = 0,

  /// A window without decorations that covers the whole display at the
  /// desktop's resolution. width and height are ignored. Switching to other
  /// applications stays instant because the display mode never changes.
  Borderless,

  /// Exclusive fullscreen. The display is switched to width by height.
  Fullscreen
};

#endif //WINDOW_MODE_HPP
