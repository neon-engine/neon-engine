#ifndef SDL_2_WITHOUT_DISPLAY_HPP
#define SDL_2_WITHOUT_DISPLAY_HPP

#include <string>

#include <gtest/gtest.h>

#include <SDL.h>

#include <neon/window/window-context.hpp>

namespace neon::testing
{
  /// A window as the input of SDL asks for it, with the sizes a test gives
  /// it. It stands in for a display of any density.
  class FakeWindow final : public WindowContext
  {
  public:
    WindowMetrics metrics{1920, 1080, 1920, 1080};
    bool was_told_to_close = false;
    bool has_focus = false;

    void SignalToClose() override
    {
      was_told_to_close = true;
    }

    double GetDeltaTime() override
    {
      return 1.0 / 60.0;
    }

    void CenterCursor() override {}

    void SetWindowFocus(const bool focus) override
    {
      has_focus = focus;
    }

    WindowSize GetDrawableSize() override
    {
      return {metrics.pixel_width, metrics.pixel_height};
    }

    WindowSize GetWindowSize() override
    {
      return {metrics.point_width, metrics.point_height};
    }

    std::vector<std::string> GetVulkanInstanceExtensions() override
    {
      return {};
    }

    bool CreateVulkanSurface(void *, void *) override
    {
      return false;
    }
  };

  /// Starts SDL with the video driver that draws nowhere, which needs no
  /// display, and creates a window with it. Events are made by the test and
  /// pushed to SDL, which hands them out as it hands out those of devices.
  ///
  /// A test is skipped where SDL was built without that driver.
  class Sdl2WithoutDisplay : public ::testing::Test
  {
  protected:
    SDL_Window *_window = nullptr;
    Uint32 _window_id = 0;
    bool _started = false;

    void SetUp() override
    {
      SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");

      if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
      {
        GTEST_SKIP() << "SDL cannot start its video driver that draws nowhere: " << SDL_GetError();
      }
      _started = true;

      const char *driver = SDL_GetCurrentVideoDriver();
      if (driver == nullptr || std::string(driver) != "dummy")
      {
        GTEST_SKIP() << "SDL started another video driver than the one that draws nowhere";
      }

      _window = SDL_CreateWindow("test", 0, 0, 640, 360, SDL_WINDOW_ALLOW_HIGHDPI);
      if (_window == nullptr) { GTEST_SKIP() << "SDL cannot create a window: " << SDL_GetError(); }

      _window_id = SDL_GetWindowID(_window);

      // what creating the window caused is not what a test is about
      SDL_PumpEvents();
      SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    }

    void TearDown() override
    {
      if (_window != nullptr) { SDL_DestroyWindow(_window); }
      _window = nullptr;

      if (_started)
      {
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
        SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
      }
      _started = false;
    }

    static void Push(SDL_Event &event)
    {
      ASSERT_EQ(SDL_PushEvent(&event), 1) << SDL_GetError();
    }

    void PushWindowEvent(const Uint8 what) const
    {
      SDL_Event event{};
      event.type = SDL_WINDOWEVENT;
      event.window.windowID = _window_id;
      event.window.event = what;
      Push(event);
    }

    void PushKey(const bool down, const SDL_Scancode scancode, const Uint16 held = KMOD_NONE, const bool repeat = false) const
    {
      SDL_Event event{};
      event.type = down ? SDL_KEYDOWN : SDL_KEYUP;
      event.key.windowID = _window_id;
      event.key.state = down ? SDL_PRESSED : SDL_RELEASED;
      event.key.repeat = repeat ? 1 : 0;
      event.key.keysym.scancode = scancode;
      event.key.keysym.sym = SDL_GetKeyFromScancode(scancode);
      event.key.keysym.mod = held;
      Push(event);
    }

    void PushMotion(const int x, const int y, const int x_relative = 0, const int y_relative = 0) const
    {
      SDL_Event event{};
      event.type = SDL_MOUSEMOTION;
      event.motion.windowID = _window_id;
      event.motion.x = x;
      event.motion.y = y;
      event.motion.xrel = x_relative;
      event.motion.yrel = y_relative;
      Push(event);
    }

    void PushButton(const bool down, const int x, const int y, const Uint8 button = SDL_BUTTON_LEFT) const
    {
      SDL_Event event{};
      event.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
      event.button.windowID = _window_id;
      event.button.button = button;
      event.button.state = down ? SDL_PRESSED : SDL_RELEASED;
      event.button.clicks = 1;
      event.button.x = x;
      event.button.y = y;
      Push(event);
    }

    void PushWheel(const float x, const float y, const bool flipped = false) const
    {
      SDL_Event event{};
      event.type = SDL_MOUSEWHEEL;
      event.wheel.windowID = _window_id;
      event.wheel.x = static_cast<Sint32>(x);
      event.wheel.y = static_cast<Sint32>(y);
      event.wheel.preciseX = x;
      event.wheel.preciseY = y;
      event.wheel.direction = flipped ? SDL_MOUSEWHEEL_FLIPPED : SDL_MOUSEWHEEL_NORMAL;
      Push(event);
    }

    void PushText(const std::string &text) const
    {
      SDL_Event event{};
      event.type = SDL_TEXTINPUT;
      event.text.windowID = _window_id;
      SDL_strlcpy(event.text.text, text.c_str(), sizeof(event.text.text));
      Push(event);
    }

    void PushComposition(const std::string &text, const int start, const int length) const
    {
      SDL_Event event{};
      event.type = SDL_TEXTEDITING;
      event.edit.windowID = _window_id;
      SDL_strlcpy(event.edit.text, text.c_str(), sizeof(event.edit.text));
      event.edit.start = start;
      event.edit.length = length;
      Push(event);
    }
  };
} // neon::testing

#endif //SDL_2_WITHOUT_DISPLAY_HPP
