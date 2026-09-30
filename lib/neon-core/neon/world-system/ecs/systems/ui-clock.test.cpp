#include "ui-clock.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-ui-system.hpp>

namespace
{
  using neon::UiClock;
  using neon::UiContext;
  using neon::testing::FakeEntityStore;
  using neon::testing::MockUiContext;

  /// A user interface that keeps the time it is told.
  class TimedUiContext : public MockUiContext
  {
  public:
    double time = 0.0;
    int told = 0;

    void AdvanceTime(const double seconds) override
    {
      time += seconds;
      told++;
    }

    [[nodiscard]] double GetTime() const override
    {
      return time;
    }
  };

  TEST(UiClockTest, MovesTheTimeOfTheUserInterfaceOnByWhatTheWorldAdvancesBy)
  {
    ::testing::NiceMock<TimedUiContext> ui;
    FakeEntityStore store;
    store.Initialize();

    UiClock clock(&ui);
    clock.Initialize(store);

    clock.Update(store, 0.25);
    clock.Update(store, 0.5);

    EXPECT_DOUBLE_EQ(ui.GetTime(), 0.75);
    EXPECT_EQ(ui.told, 2) << "once in a frame";

    store.CleanUp();
  }

  TEST(UiClockTest, AUserInterfaceThatKeepsNoTimeIsLeftAlone)
  {
    ::testing::StrictMock<MockUiContext> ui;
    FakeEntityStore store;
    store.Initialize();

    UiClock clock(&ui);
    clock.Initialize(store);
    clock.Update(store, 0.25);

    const UiContext &context = ui;
    EXPECT_DOUBLE_EQ(context.GetTime(), 0.0);

    store.CleanUp();
  }
}
