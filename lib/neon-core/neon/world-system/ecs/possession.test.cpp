#include "possession.hpp"

#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/recording-logger.hpp>

#include "components/first-person-controller.hpp"
#include "components/possessable.hpp"
#include "components/possessed.hpp"
#include "components/spectator.hpp"

namespace
{
  using neon::Entity;
  using neon::FirstPersonController;
  using neon::No_Entity;
  using neon::Possessable;
  using neon::Possessed;
  using neon::Possession;
  using neon::PossessionEvent;
  using neon::PossessionListener;
  using neon::Spectator;
  using neon::testing::FakeEntityStore;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;

  /// Writes down what it is told.
  class RecordingListener final : public PossessionListener
  {
  public:
    std::vector<PossessionEvent> events;

    void OnPossession(const PossessionEvent &event) override
    {
      events.push_back(event);
    }
  };

  class PossessionTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    Possession _possession{&_store, _logger};
    RecordingListener _listener;

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Possessable>("Possessable");
      _store.Register<Possessed>("Possessed");
      _store.Register<FirstPersonController>("FirstPersonController");
      _store.Register<Spectator>("Spectator");
      _possession.AddListener(&_listener);
    }

    Entity Possessable_(const std::string &name, const Entity parent = No_Entity)
    {
      const Entity entity = _store.CreateEntity(name, parent);
      _store.Set(entity, Possessable{});
      return entity;
    }

    [[nodiscard]] std::string Errors() const
    {
      return _logger->Messages(LogLevel::Error);
    }
  };

  TEST_F(PossessionTest, NothingIsPossessedAtFirst)
  {
    EXPECT_EQ(_possession.GetPossessed(), No_Entity);
  }

  TEST_F(PossessionTest, PossessesAnEntityThatCanBeAndTellsOfIt)
  {
    const Entity cart = Possessable_("cart");

    EXPECT_TRUE(_possession.Possess(cart));

    EXPECT_EQ(_possession.GetPossessed(), cart);
    EXPECT_TRUE(_store.Has<Possessed>(cart));
    ASSERT_EQ(_listener.events.size(), 1u);
    EXPECT_EQ(_listener.events[0].entity, cart);
    EXPECT_TRUE(_listener.events[0].possessed);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Possessed 'cart'"));
  }

  TEST_F(PossessionTest, RefusesAnEntityThatCarriesNoPossessable)
  {
    const Entity rock = _store.CreateEntity("rock");

    EXPECT_FALSE(_possession.Possess(rock));

    EXPECT_EQ(_possession.GetPossessed(), No_Entity);
    EXPECT_FALSE(_store.Has<Possessed>(rock));
    EXPECT_TRUE(_listener.events.empty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "'rock' cannot be possessed: it carries no Possessable")) << Errors();
  }

  TEST_F(PossessionTest, RefusesAnEntityThatIsGone)
  {
    const Entity cart = Possessable_("cart");
    _store.DestroyEntity(cart);

    EXPECT_FALSE(_possession.Possess(cart));
    EXPECT_FALSE(_possession.Possess(No_Entity));

    EXPECT_EQ(_logger->Count(LogLevel::Error), 2u) << Errors();
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "An entity that is gone cannot be possessed"));
  }

  TEST_F(PossessionTest, ReleasesTheEntityBeforeAndTellsOfBoth)
  {
    const Entity player = Possessable_("player");
    const Entity cart = Possessable_("cart");
    ASSERT_TRUE(_possession.Possess(player));
    _listener.events.clear();

    EXPECT_TRUE(_possession.Possess(cart));

    EXPECT_EQ(_possession.GetPossessed(), cart);
    EXPECT_FALSE(_store.Has<Possessed>(player));
    EXPECT_TRUE(_store.Has<Possessed>(cart));
    ASSERT_EQ(_listener.events.size(), 2u);
    EXPECT_EQ(_listener.events[0].entity, player);
    EXPECT_FALSE(_listener.events[0].possessed);
    EXPECT_EQ(_listener.events[1].entity, cart);
    EXPECT_TRUE(_listener.events[1].possessed);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Released 'player'"));
  }

  TEST_F(PossessionTest, PossessingThePossessedEntityChangesNothing)
  {
    const Entity cart = Possessable_("cart");
    ASSERT_TRUE(_possession.Possess(cart));
    _listener.events.clear();

    EXPECT_TRUE(_possession.Possess(cart));

    EXPECT_TRUE(_listener.events.empty());
  }

  TEST_F(PossessionTest, UnpossessReleasesSoThatNothingIsPossessed)
  {
    const Entity cart = Possessable_("cart");
    ASSERT_TRUE(_possession.Possess(cart));
    _listener.events.clear();

    _possession.Unpossess();
    _possession.Unpossess();

    EXPECT_EQ(_possession.GetPossessed(), No_Entity);
    EXPECT_FALSE(_store.Has<Possessed>(cart));
    ASSERT_EQ(_listener.events.size(), 1u);
    EXPECT_EQ(_listener.events[0].entity, cart);
    EXPECT_FALSE(_listener.events[0].possessed);
  }

  TEST_F(PossessionTest, NothingIsPossessedOnceThePossessedEntityIsGone)
  {
    const Entity cart = Possessable_("cart");
    ASSERT_TRUE(_possession.Possess(cart));

    _store.DestroyEntity(cart);

    EXPECT_EQ(_possession.GetPossessed(), No_Entity);
  }

  TEST_F(PossessionTest, AnotherEntityIsPossessedAfterThePossessedOneIsGone)
  {
    const Entity cart = Possessable_("cart");
    const Entity player = Possessable_("player");
    ASSERT_TRUE(_possession.Possess(cart));
    _store.DestroyEntity(cart);
    _listener.events.clear();

    EXPECT_TRUE(_possession.Possess(player));

    // nobody is told of what is gone
    ASSERT_EQ(_listener.events.size(), 1u);
    EXPECT_EQ(_listener.events[0].entity, player);
  }

  // what is possessed when a scene starts

  TEST_F(PossessionTest, StartTakesTheEntityTheSceneNamesWhateverItCarries)
  {
    Possessable_("player");
    const Entity rock = _store.CreateEntity("rock");

    _possession.Start("rock");

    EXPECT_EQ(_possession.GetPossessed(), rock);
    EXPECT_TRUE(_store.Has<Possessed>(rock));
  }

  TEST_F(PossessionTest, StartFindsTheNamedEntityByItsPath)
  {
    const Entity hall = _store.CreateEntity("hall");
    const Entity cart = Possessable_("cart", hall);

    _possession.Start("hall/cart");

    EXPECT_EQ(_possession.GetPossessed(), cart);
  }

  TEST_F(PossessionTest, StartSaysWhenTheNamedEntityIsNotThereAndTakesWhatItWould)
  {
    const Entity player = Possessable_("player");

    _possession.Start("car");

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The scene possesses 'car', and there is no such entity")) << Errors();
    EXPECT_EQ(_possession.GetPossessed(), player);
  }

  TEST_F(PossessionTest, StartTakesTheFirstPossessableInTheOrderOfTheScene)
  {
    _store.CreateEntity("floor");
    const Entity hall = _store.CreateEntity("hall");
    const Entity cart = Possessable_("cart", hall);
    Possessable_("player");

    _possession.Start("");

    EXPECT_EQ(_possession.GetPossessed(), cart);
  }

  TEST_F(PossessionTest, StartTakesThePlayerOfASceneWrittenBeforeTheTag)
  {
    const Entity spectator = _store.CreateEntity("spectator");
    _store.Set(spectator, Spectator{});
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, FirstPersonController{});

    _possession.Start("");

    EXPECT_EQ(_possession.GetPossessed(), player);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(PossessionTest, StartTakesTheSpectatorWithoutAPlayer)
  {
    const Entity spectator = _store.CreateEntity("spectator");
    _store.Set(spectator, Spectator{});

    _possession.Start("");

    EXPECT_EQ(_possession.GetPossessed(), spectator);
  }

  TEST_F(PossessionTest, StartPrefersAPossessableToAPlayer)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, FirstPersonController{});
    const Entity cart = Possessable_("cart");

    _possession.Start("");

    EXPECT_EQ(_possession.GetPossessed(), cart);
  }

  TEST_F(PossessionTest, StartTakesNothingInASceneWithoutAnything)
  {
    _store.CreateEntity("floor");

    _possession.Start("");

    EXPECT_EQ(_possession.GetPossessed(), No_Entity);
    EXPECT_TRUE(_listener.events.empty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Nothing is possessed"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(PossessionTest, StartKeepsWhatStayedThroughTheChangeOfScene)
  {
    const Entity player = Possessable_("player");
    ASSERT_TRUE(_possession.Possess(player));
    Possessable_("cart");
    _listener.events.clear();

    _possession.Start("");

    EXPECT_EQ(_possession.GetPossessed(), player);
    EXPECT_TRUE(_listener.events.empty());
  }

  TEST_F(PossessionTest, StartTakesAnotherOnceWhatWasPossessedIsGone)
  {
    const Entity player = Possessable_("player");
    ASSERT_TRUE(_possession.Possess(player));
    _store.DestroyEntity(player);
    const Entity cart = Possessable_("cart");

    _possession.Start("");

    EXPECT_EQ(_possession.GetPossessed(), cart);
  }

  TEST_F(PossessionTest, StartTakesTheNamedEntityOverWhatStayed)
  {
    const Entity player = Possessable_("player");
    ASSERT_TRUE(_possession.Possess(player));
    const Entity cart = Possessable_("cart");

    _possession.Start("cart");

    EXPECT_EQ(_possession.GetPossessed(), cart);
    EXPECT_FALSE(_store.Has<Possessed>(player));
  }
} // namespace
