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

    /// Creates `count` components in memory that holds none. The four
    /// functions may hold state, so that a component whose layout is only
    /// known while the game runs, one a script declares, can be described
    /// too.
    std::function<void(void *at, std::size_t count)> construct;

    /// Ends `count` components. The memory is released by the store.
    std::function<void(void *at, std::size_t count)> destruct;

    /// Assigns `count` components from others that stay as they are.
    std::function<void(void *to, const void *from, std::size_t count)> copy;

    /// Assigns `count` components from others that are given up.
    std::function<void(void *to, void *from, std::size_t count)> move;

    /// Called before a component leaves an entity, which includes the entity
    /// being destroyed and the store being cleaned up. It is the place to
    /// release what the component refers to outside the store.
    std::function<void(Entity entity, void *component)> on_remove;

    /// Called when a component of an entity is turned off or on, see
    /// EntityStore::SetEnabled(). A component that keeps something outside
    /// the store, a body of the physics, lets go of it when it is turned
    /// off, as it does when it is removed, and takes it again when it is
    /// turned on. A component that only holds what systems read needs
    /// none: it is passed over while it is off.
    std::function<void(Entity entity, void *component, bool enabled)> on_toggle;

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
