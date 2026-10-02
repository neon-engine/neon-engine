#ifndef MOCK_UI_SYSTEM_HPP
#define MOCK_UI_SYSTEM_HPP

#include <functional>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <neon/ui/ui-system.hpp>

namespace neon::testing
{
  class MockUiContext : public UiContext
  {
  public:
    MOCK_METHOD(int, Load, (const std::string &path), (override));

    MOCK_METHOD(void, Unload, (int document), (override));

    MOCK_METHOD(bool, IsShown, (int document), (const, override));

    MOCK_METHOD(void, SetNumber, (const std::string &name, double number), (override));

    MOCK_METHOD(void, SetText, (const std::string &name, const std::string &text), (override));

    MOCK_METHOD(void, SetFlag, (const std::string &name, bool flag), (override));

    MOCK_METHOD(std::string, GetValue, (const std::string &name, bool *is_set), (const, override));

    MOCK_METHOD(bool, GetNumber, (const std::string &name, double &number), (const, override));

    MOCK_METHOD(void, OnClick, (const std::string &element, const std::function<void()> &callback), (override));

    MOCK_METHOD(const std::vector<UiEvent> &, GetEvents, (), (const, override));

    MOCK_METHOD(bool, WasClicked, (const std::string &element), (const, override));

    MOCK_METHOD(bool, Focus, (const std::string &element), (override));

    MOCK_METHOD(std::string, GetFocused, (), (const, override));

    // surfaces in the world

    MOCK_METHOD(int, CreateSurface, (const std::string &name, int width, int height, float scale), (override));

    MOCK_METHOD(void, DestroySurface, (int surface), (override));

    MOCK_METHOD(int, FindSurface, (const std::string &name), (const, override));

    MOCK_METHOD(int, LoadOnto, (int surface, const std::string &path), (override));

    MOCK_METHOD(void, SetPointer, (int surface, float x, float y, bool is_down), (override));

    MOCK_METHOD(void, SetPointerUv, (int surface, float u, float v, bool is_down), (override));

    MOCK_METHOD(void, ClearPointer, (int surface), (override));
  };

  class MockUiSystem : public UiSystem
  {
  public:
    MOCK_METHOD(void, Initialize, (), (override));

    MOCK_METHOD(void, Update, (), (override));

    MOCK_METHOD(void, Draw, (), (override));

    MOCK_METHOD(void, CleanUp, (), (override));

    MOCK_METHOD(InputContext *, GetGameInput, (), (override));

    MOCK_METHOD(int, Load, (const std::string &path), (override));

    MOCK_METHOD(void, Unload, (int document), (override));

    MOCK_METHOD(bool, IsShown, (int document), (const, override));

    MOCK_METHOD(void, SetNumber, (const std::string &name, double number), (override));

    MOCK_METHOD(void, SetText, (const std::string &name, const std::string &text), (override));

    MOCK_METHOD(void, SetFlag, (const std::string &name, bool flag), (override));

    MOCK_METHOD(void, OnClick, (const std::string &element, const std::function<void()> &callback), (override));

    MOCK_METHOD(const std::vector<UiEvent> &, GetEvents, (), (const, override));

    MOCK_METHOD(bool, WasClicked, (const std::string &element), (const, override));

    MOCK_METHOD(bool, Focus, (const std::string &element), (override));

    MOCK_METHOD(std::string, GetFocused, (), (const, override));
  };
} // neon::testing

#endif //MOCK_UI_SYSTEM_HPP
