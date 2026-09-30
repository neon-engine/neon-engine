#include "component-info.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
  using neon::ComponentInfo;

  /// Owns memory, which a component that is handled byte by byte would lose
  /// or release twice.
  struct Named
  {
    std::string name = "a name that is given to every component that was just created";
    std::vector<int> numbers{1, 2, 3};
  };

  struct alignas(32) Aligned
  {
    float values[8];
  };

  /// Counts what happens to it.
  struct Counted
  {
    static inline int constructed = 0;
    static inline int destructed = 0;
    static inline int copied = 0;
    static inline int moved = 0;

    int value = 7;

    Counted() { constructed++; }

    ~Counted() { destructed++; }

    Counted &operator=(const Counted &other)
    {
      value = other.value;
      copied++;
      return *this;
    }

    Counted &operator=(Counted &&other) noexcept
    {
      value = other.value;
      other.value = -1;
      moved++;
      return *this;
    }

    static void Reset()
    {
      constructed = destructed = copied = moved = 0;
    }
  };

  /// Memory for components that holds none, as a store sets it aside.
  template<typename T, std::size_t Count>
  struct Memory
  {
    alignas(T) unsigned char bytes[sizeof(T) * Count];

    T *At(const std::size_t index = 0)
    {
      return reinterpret_cast<T *>(bytes) + index;
    }
  };

  const std::string long_text = "a text that is too long to be kept inside of the string itself";

  TEST(ComponentInfo, IsEmptyUntilItIsFilledIn)
  {
    const ComponentInfo info;

    EXPECT_EQ(info.name, "");
    EXPECT_EQ(info.size, 0u);
    EXPECT_EQ(info.alignment, 0u);
    EXPECT_EQ(info.construct, nullptr);
    EXPECT_EQ(info.destruct, nullptr);
    EXPECT_EQ(info.copy, nullptr);
    EXPECT_EQ(info.move, nullptr);
    EXPECT_FALSE(info.on_remove);
  }

  TEST(ComponentInfo, TakesNameSizeAndAlignmentFromTheType)
  {
    const auto named = ComponentInfo::Of<Named>("Named");
    EXPECT_EQ(named.name, "Named");
    EXPECT_EQ(named.size, sizeof(Named));
    EXPECT_EQ(named.alignment, alignof(Named));

    const auto aligned = ComponentInfo::Of<Aligned>("Aligned");
    EXPECT_EQ(aligned.size, sizeof(Aligned));
    EXPECT_EQ(aligned.alignment, 32u);

    const auto number = ComponentInfo::Of<char>("Char");
    EXPECT_EQ(number.size, 1u);
    EXPECT_EQ(number.alignment, 1u);
  }

  TEST(ComponentInfo, FillsInEveryFunctionButOnRemove)
  {
    const auto info = ComponentInfo::Of<Named>("Named");

    EXPECT_NE(info.construct, nullptr);
    EXPECT_NE(info.destruct, nullptr);
    EXPECT_NE(info.copy, nullptr);
    EXPECT_NE(info.move, nullptr);
    EXPECT_FALSE(info.on_remove);
  }

  TEST(ComponentInfo, ConstructsComponentsAsTheTypeDoesByDefault)
  {
    const auto info = ComponentInfo::Of<Named>("Named");
    Memory<Named, 3> memory;

    info.construct(memory.At(), 3);

    for (std::size_t i = 0; i < 3; i++)
    {
      EXPECT_EQ(memory.At(i)->name, Named{}.name);
      EXPECT_EQ(memory.At(i)->numbers, (std::vector{1, 2, 3}));
    }

    info.destruct(memory.At(), 3);
  }

  TEST(ComponentInfo, ConstructsANumberAsZero)
  {
    const auto info = ComponentInfo::Of<int>("Number");
    Memory<int, 2> memory;
    *memory.At(0) = 123;
    *memory.At(1) = 456;

    info.construct(memory.At(), 2);

    EXPECT_EQ(*memory.At(0), 0);
    EXPECT_EQ(*memory.At(1), 0);
  }

  TEST(ComponentInfo, ConstructsAndDestructsAsManyAsItIsTold)
  {
    Counted::Reset();
    const auto info = ComponentInfo::Of<Counted>("Counted");
    Memory<Counted, 5> memory;

    info.construct(memory.At(), 5);
    EXPECT_EQ(Counted::constructed, 5);
    EXPECT_EQ(Counted::destructed, 0);

    info.destruct(memory.At(), 2);
    EXPECT_EQ(Counted::destructed, 2);

    info.destruct(memory.At(2), 3);
    EXPECT_EQ(Counted::destructed, 5);
    EXPECT_EQ(Counted::constructed, 5);
  }

  TEST(ComponentInfo, DoesNothingForACountOfZero)
  {
    Counted::Reset();
    const auto info = ComponentInfo::Of<Counted>("Counted");
    Memory<Counted, 1> memory;

    info.construct(memory.At(), 0);
    info.copy(memory.At(), memory.At(), 0);
    info.move(memory.At(), memory.At(), 0);
    info.destruct(memory.At(), 0);

    EXPECT_EQ(Counted::constructed, 0);
    EXPECT_EQ(Counted::copied, 0);
    EXPECT_EQ(Counted::moved, 0);
    EXPECT_EQ(Counted::destructed, 0);
  }

  TEST(ComponentInfo, CopiesComponentsAndLeavesTheSourceAsItIs)
  {
    const auto info = ComponentInfo::Of<Named>("Named");
    Named source[2];
    source[0].name = long_text + " one";
    source[0].numbers = {4, 5};
    source[1].name = long_text + " two";
    source[1].numbers = {};
    Named target[2];

    info.copy(target, source, 2);

    EXPECT_EQ(target[0].name, long_text + " one");
    EXPECT_EQ(target[0].numbers, (std::vector{4, 5}));
    EXPECT_EQ(target[1].name, long_text + " two");
    EXPECT_TRUE(target[1].numbers.empty());

    EXPECT_EQ(source[0].name, long_text + " one");
    EXPECT_EQ(source[0].numbers, (std::vector{4, 5}));
  }

  TEST(ComponentInfo, CopiesIntoAComponentOfItsOwn)
  {
    const auto info = ComponentInfo::Of<Named>("Named");
    Named source;
    source.name = long_text;
    Named target;

    info.copy(&target, &source, 1);
    source.name[0] = 'X';
    source.numbers.push_back(4);

    EXPECT_EQ(target.name, long_text);
    EXPECT_EQ(target.numbers, (std::vector{1, 2, 3}));
  }

  TEST(ComponentInfo, MovesComponents)
  {
    const auto info = ComponentInfo::Of<Named>("Named");
    Named source[2];
    source[0].name = long_text + " one";
    source[0].numbers = {4, 5};
    source[1].name = long_text + " two";
    Named target[2];

    info.move(target, source, 2);

    EXPECT_EQ(target[0].name, long_text + " one");
    EXPECT_EQ(target[0].numbers, (std::vector{4, 5}));
    EXPECT_EQ(target[1].name, long_text + " two");
  }

  TEST(ComponentInfo, MovesByMovingAndCopiesByCopying)
  {
    Counted::Reset();
    const auto info = ComponentInfo::Of<Counted>("Counted");
    Counted source[3];
    Counted target[3];
    source[1].value = 42;

    info.copy(target, source, 3);
    EXPECT_EQ(Counted::copied, 3);
    EXPECT_EQ(Counted::moved, 0);
    EXPECT_EQ(target[1].value, 42);
    EXPECT_EQ(source[1].value, 42);

    info.move(target, source, 3);
    EXPECT_EQ(Counted::copied, 3);
    EXPECT_EQ(Counted::moved, 3);
    EXPECT_EQ(target[1].value, 42);
    EXPECT_EQ(source[1].value, -1);
  }

  TEST(ComponentInfo, LeavesAComponentThatWasMovedFromFitToBeDestructed)
  {
    const auto info = ComponentInfo::Of<Named>("Named");
    Memory<Named, 2> from;
    Memory<Named, 2> to;
    info.construct(from.At(), 2);
    info.construct(to.At(), 2);
    from.At(0)->name = long_text;

    // what a store does when it moves entities to other memory
    info.move(to.At(), from.At(), 2);
    info.destruct(from.At(), 2);

    EXPECT_EQ(to.At(0)->name, long_text);
    EXPECT_EQ(to.At(1)->numbers, (std::vector{1, 2, 3}));

    info.destruct(to.At(), 2);
  }

  TEST(ComponentInfo, WorksOnTheComponentsItIsPointedAtOnly)
  {
    const auto info = ComponentInfo::Of<Named>("Named");
    Named source;
    source.name = long_text;
    Named target[3];

    info.copy(&target[1], &source, 1);

    EXPECT_EQ(target[0].name, Named{}.name);
    EXPECT_EQ(target[1].name, long_text);
    EXPECT_EQ(target[2].name, Named{}.name);
  }

  TEST(ComponentInfo, KeepsTheTypesApart)
  {
    const auto named = ComponentInfo::Of<Named>("Named");
    const auto counted = ComponentInfo::Of<Counted>("Counted");

    EXPECT_NE(named.size, counted.size);
    EXPECT_NE(named.construct, counted.construct);
    EXPECT_NE(named.destruct, counted.destruct);
  }

  TEST(ComponentInfo, CanBeCopiedWithItsFunctions)
  {
    auto info = ComponentInfo::Of<Named>("Named");
    int removed = 0;
    info.on_remove = [&removed](neon::Entity, void *) { removed++; };

    const ComponentInfo copy = info;
    Named component;
    copy.on_remove(1, &component);

    EXPECT_EQ(copy.name, "Named");
    EXPECT_EQ(copy.construct, info.construct);
    EXPECT_EQ(removed, 1);
  }
}
