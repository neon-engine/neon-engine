#ifndef WINDOW_INFO_HPP
#define WINDOW_INFO_HPP

#include <string>

enum class RenderingApi
{
  OpenGl = 0
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
  RenderingApi selected_api;
  WindowMode window_mode = WindowMode::Windowed;
  std::string logpath = "logs/neon-engine.log";
  std::size_t log_max_size = 1048576 * 5;
  std::size_t log_max_files = 1;
  std::size_t max_light_sources = 1024;
};

#endif //WINDOW_INFO_HPP
