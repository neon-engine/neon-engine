#ifndef WINDOW_INFO_HPP
#define WINDOW_INFO_HPP

#include <string>

enum class RenderingApi
{
  Vulkan = 0
};

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

struct SettingsConfig
{
  int width;
  int height;
  std::string title;

  /// Who makes the application, and what it is called. Together they decide
  /// where `user://` lives, so that applications do not share a folder. Use
  /// plain lowercase names without spaces.
  std::string organization = "neon-engine";
  std::string application = "neon-runtime";

  RenderingApi selected_api;
  WindowMode window_mode = WindowMode::Windowed;
  std::string logpath = "logs/neon-engine.log";
  std::size_t log_max_size = 1048576 * 5;
  std::size_t log_max_files = 1;
  std::size_t max_light_sources = 1024;

  /// Run without a window and without input devices.
  bool headless = false;

  /// Number of frames to render before the application stops by itself.
  /// 0 keeps it running until its window is closed.
  std::size_t max_frames = 0;

  /// Virtual path the last frame is saved to as a PNG image before the
  /// application stops, such as `user://screenshots/frame.png`. Only used
  /// together with max_frames. Empty saves nothing.
  std::string screenshot_path;
};

#endif //WINDOW_INFO_HPP
