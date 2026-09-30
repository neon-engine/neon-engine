#ifndef WINDOW_INFO_HPP
#define WINDOW_INFO_HPP

#include <cstddef>
#include <string>
#include <vector>

enum class RenderingApi
{
  Vulkan = 0
};

enum class AudioOutput
{
  /// The sound card the system plays through.
  Device = 0,

  /// Nowhere. Sounds are read and mixed as they are for a sound card, at the
  /// pace the application advances and not the pace of a clock. For runs
  /// without a sound card that still want to know what would be heard.
  None
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

  /// Virtual path of the scene the application starts with.
  std::string scene_path = "assets://scenes/demo.scene.yml";

  /// Virtual path of a user interface that is shown from the start, on top
  /// of what the scene shows. Empty shows none.
  std::string ui_path;

  RenderingApi selected_api;
  AudioOutput audio_output = AudioOutput::Device;
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

  /// Native folder behind `output://`, as it was given by whoever started
  /// the application. It is absolute or relative to the working directory.
  /// This is the only native path the settings carry, and only a file system
  /// backend reads it. Empty leaves `output://` without a folder, and every
  /// path in it is rejected.
  std::string output_directory;

  /// Seconds the application advances in every frame, whatever time the
  /// frame took. The same run then gives the same frames on every machine.
  /// 0 advances by the time that was measured. Without a window there is
  /// nothing to measure against, and 0 stands for a sixtieth of a second.
  double time_step = 0.0;

  /// Steps the world takes in a second. What a game is decided by, the
  /// physics first of all, advances in steps of this length, whatever the
  /// frame rate is.
  double steps_per_second = 60.0;

  /// The most steps one frame takes. A frame that took longer gives up the
  /// time above them, and the world runs behind the clock for a moment.
  std::size_t most_steps_per_frame = 8;

  /// Virtual path the last frame is saved to as a PNG image before the
  /// application stops, such as `output://frame.png`. Only used
  /// together with max_frames. Empty saves nothing.
  std::string screenshot_path;

  /// Frames to save instead of the last one, counted from 1 and in rising
  /// order. Each is saved to screenshot_path with its number in front of the
  /// extension, such as `output://frame-0030.png`. None is above max_frames.
  std::vector<std::size_t> screenshot_frames;
};

#endif //WINDOW_INFO_HPP
