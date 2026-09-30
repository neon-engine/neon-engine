#ifndef SDL_2_CLIPBOARD_HPP
#define SDL_2_CLIPBOARD_HPP

#include <neon/input/clipboard-context.hpp>

namespace neon
{
  /// The clipboard of the platform, through SDL. It needs the video
  /// subsystem of SDL, which a window brings.
  // ReSharper disable once CppInconsistentNaming
  class SDL2_Clipboard final : public ClipboardContext
  {
  public:
    [[nodiscard]] bool HasText() override;

    [[nodiscard]] std::string GetText() override;

    bool SetText(const std::string &text) override;
  };
} // neon

#endif //SDL_2_CLIPBOARD_HPP
