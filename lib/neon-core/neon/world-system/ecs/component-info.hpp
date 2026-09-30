#ifndef COMPONENT_INFO_HPP
#define COMPONENT_INFO_HPP

#include <cstddef>
#include <functional>
#include <new>
#include <string>
#include <utility>

#include "entity.hpp"

namespace neon
{
  /// Describes a kind of component to the entity store.
  ///
  /// A store knows components by their size and by these functions alone. It
  /// never sees a C++ type, which is what lets a component be declared by
  /// anything that can fill this in: engine code, a game, or a script.
  struct ComponentInfo
  {
    /// Unique among components, such as `Transform`.
    std::string name;

    std::size_t size = 0;
    std::size_t alignment = 0;

    /// Creates `count` components in memory that holds none.
    void (*construct)(void *at, std::size_t count) = nullptr;

    /// Ends `count` components. The memory is released by the store.
    void (*destruct)(void *at, std::size_t count) = nullptr;

    /// Assigns `count` components from others that stay as they are.
    void (*copy)(void *to, const void *from, std::size_t count) = nullptr;

    /// Assigns `count` components from others that are given up.
    void (*move)(void *to, void *from, std::size_t count) = nullptr;

    /// Called before a component leaves an entity, which includes the entity
    /// being destroyed and the store being cleaned up. It is the place to
    /// release what the component refers to outside the store.
    std::function<void(Entity entity, void *component)> on_remove;

    /// Fills everything in from a C++ type.
    template<typename T>
    static ComponentInfo Of(const std::string &name)
    {
      ComponentInfo info;
      info.name = name;
      info.size = sizeof(T);
      info.alignment = alignof(T);

      info.construct = [](void *at, const std::size_t count)
      {
        auto *components = static_cast<T *>(at);
        for (std::size_t i = 0; i < count; i++) { new(&components[i]) T(); }
      };

      info.destruct = [](void *at, const std::size_t count)
      {
        auto *components = static_cast<T *>(at);
        for (std::size_t i = 0; i < count; i++) { components[i].~T(); }
      };

      info.copy = [](void *to, const void *from, const std::size_t count)
      {
        auto *target = static_cast<T *>(to);
        const auto *source = static_cast<const T *>(from);
        for (std::size_t i = 0; i < count; i++) { target[i] = source[i]; }
      };

      info.move = [](void *to, void *from, const std::size_t count)
      {
        auto *target = static_cast<T *>(to);
        auto *source = static_cast<T *>(from);
        for (std::size_t i = 0; i < count; i++) { target[i] = std::move(source[i]); }
      };

      return info;
    }
  };
} // neon

#endif //COMPONENT_INFO_HPP
