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

    MOCK_METHOD(void, SetNumber, (const std::string &name, double number), (override));

    MOCK_METHOD(void, SetText, (const std::string &name, const std::string &text), (override));

    MOCK_METHOD(void, SetFlag, (const std::string &name, bool flag), (override));

    MOCK_METHOD(void, OnClick, (const std::string &element, const std::function<void()> &callback), (override));

    MOCK_METHOD(const std::vector<UiEvent> &, GetEvents, (), (const, override));

    MOCK_METHOD(bool, WasClicked, (const std::string &element), (const, override));

    MOCK_METHOD(bool, Focus, (const std::string &element), (override));

    MOCK_METHOD(std::string, GetFocused, (), (const, override));
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
