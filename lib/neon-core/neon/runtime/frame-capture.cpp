#include "frame-capture.hpp"

#include <algorithm>
#include <format>

namespace neon
{
  FrameCapture::FrameCapture(const SettingsConfig &settings_config, RenderSystem *render_system)
  {
    _settings_config = settings_config;
    _render_system = render_system;
  }

  bool FrameCapture::AfterFrame(const std::size_t frame) const
  {
    if (_settings_config.screenshot_path.empty()) { return true; }

    const auto &frames = _settings_config.screenshot_frames;
    if (frames.empty())
    {
      // only the last frame of the run
      if (_settings_config.max_frames == 0 || frame != _settings_config.max_frames) { return true; }
      return _render_system->CaptureFrame(_settings_config.screenshot_path);
    }

    if (std::ranges::find(frames, frame) == frames.end()) { return true; }
    return _render_system->CaptureFrame(NumberedPath(_settings_config.screenshot_path, frame));
  }

  std::string FrameCapture::NumberedPath(const std::string &path, const std::size_t frame)
  {
    const std::string number = std::format("-{:04}", frame);

    // the name of the file starts after the last slash, which the scheme
    // provides when there are no folders
    const std::size_t slash = path.rfind('/');
    const std::size_t name = slash == std::string::npos ? 0 : slash + 1;
    const std::size_t dot = path.rfind('.');

    // a dot that starts the name is part of the name, not an extension
    if (dot == std::string::npos || dot <= name) { return path + number; }

    return path.substr(0, dot) + number + path.substr(dot);
  }
} // neon
