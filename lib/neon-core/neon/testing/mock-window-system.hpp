#ifndef MOCK_WINDOW_SYSTEM_HPP
#define MOCK_WINDOW_SYSTEM_HPP

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <neon/window/window-system.hpp>

namespace neon::testing
{
  /// Stands in for a window where only WindowContext is asked for.
  class MockWindowContext : public WindowContext
  {
  public:
    MOCK_METHOD(void, SignalToClose, (), (override));

    MOCK_METHOD(double, GetDeltaTime, (), (override));

    MOCK_METHOD(void, CenterCursor, (), (override));

    MOCK_METHOD(void, SetWindowFocus, (bool focus), (override));

    MOCK_METHOD(WindowSize, GetDrawableSize, (), (override));

    MOCK_METHOD(std::vector<std::string>, GetVulkanInstanceExtensions, (), (override));

    MOCK_METHOD(bool, CreateVulkanSurface, (void *instance, void *surface), (override));
  };

  class MockWindowSystem : public WindowSystem
  {
  public:
    explicit MockWindowSystem(const std::shared_ptr<Logger> &logger, const SettingsConfig &settings_config = {})
      : WindowSystem(settings_config, logger) {}

    MOCK_METHOD(void, ConfigureWindowForRenderer, (), (override));

    MOCK_METHOD(void, Initialize, (), (override));

    MOCK_METHOD(bool, IsRunning, (), (const, override));

    MOCK_METHOD(void, CleanUp, (), (override));

    MOCK_METHOD(void, Update, (), (override));

    MOCK_METHOD(void, SignalToClose, (), (override));

    MOCK_METHOD(double, GetDeltaTime, (), (override));

    MOCK_METHOD(void, CenterCursor, (), (override));

    MOCK_METHOD(void, SetWindowFocus, (bool focus), (override));

    MOCK_METHOD(WindowSize, GetDrawableSize, (), (override));

    MOCK_METHOD(std::vector<std::string>, GetVulkanInstanceExtensions, (), (override));

    MOCK_METHOD(bool, CreateVulkanSurface, (void *instance, void *surface), (override));
  };
} // neon::testing

#endif //MOCK_WINDOW_SYSTEM_HPP
