#ifndef FRAME_CAPTURE_HPP
#define FRAME_CAPTURE_HPP

#include <cstddef>
#include <string>

#include "settings-config.hpp"
#include "neon/render/render-system.hpp"

namespace neon
{
  /// Saves the frames a run was asked to save.
  ///
  /// Without a list of frames, the last frame of the run is saved under the
  /// exact name that was given. With one, each frame of the list is saved
  /// under a name that carries its number, so `output://frame.png` becomes
  /// `output://frame-0030.png` for frame 30.
  class FrameCapture final
  {
    SettingsConfig _settings_config;
    RenderSystem *_render_system;

  public:
    FrameCapture(const SettingsConfig &settings_config, RenderSystem *render_system);

    /// Called after every finished frame. Frames are counted from 1. Returns
    /// false when the frame had to be saved and could not be.
    [[nodiscard]] bool AfterFrame(std::size_t frame) const;

    /// The virtual path a numbered frame is saved to. The number goes in
    /// front of the extension of the file, with at least four digits.
    [[nodiscard]] static std::string NumberedPath(const std::string &path, std::size_t frame);
  };
} // neon

#endif //FRAME_CAPTURE_HPP
