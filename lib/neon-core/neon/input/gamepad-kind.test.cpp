#include "gamepad-kind.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::FindGamepadKind;
  using neon::GamepadKind;
  using neon::NameOf;

  TEST(GamepadKindTest, EveryKindHasANameThatLeadsBackToIt)
  {
    for (const GamepadKind kind : {
           GamepadKind::Other, GamepadKind::Xbox, GamepadKind::PlayStation4, GamepadKind::PlayStation5,
           GamepadKind::Switch})
    {
      GamepadKind found = GamepadKind::Other;
      EXPECT_TRUE(FindGamepadKind(NameOf(kind), found)) << NameOf(kind);
      EXPECT_EQ(found, kind);
    }

    EXPECT_EQ(NameOf(GamepadKind::PlayStation5), "playstation5");
    EXPECT_EQ(NameOf(GamepadKind::Switch), "switch");
  }

  TEST(GamepadKindTest, ANameThatIsNoneIsNotFoundAndChangesNothing)
  {
    GamepadKind kind = GamepadKind::Xbox;
    EXPECT_FALSE(FindGamepadKind("dreamcast", kind));
    EXPECT_FALSE(FindGamepadKind("", kind));
    EXPECT_EQ(kind, GamepadKind::Xbox);
  }
}
