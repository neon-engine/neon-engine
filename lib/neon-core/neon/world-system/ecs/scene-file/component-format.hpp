#ifndef COMPONENT_FORMAT_HPP
#define COMPONENT_FORMAT_HPP

#include <functional>
#include <string>
#include <vector>

#include <neon/data/data-reader.hpp>
#include <neon/data/data-value.hpp>
#include <neon/world-system/ecs/entity-store.hpp>

namespace neon
{
  /// How a kind of component is written in a scene file.
  struct ComponentFormat
  {
    /// What the component is called in a scene file, such as `Transform`.
    /// It is the name the component was registered under.
    std::string name;

    /// Reads a component and gives it to the entity.
    std::function<void(const DataReader &reader, EntityStore &store, Entity entity)> read;

    /// Writes the component of the entity into `value`. Returns false when
    /// the entity carries none.
    std::function<bool(EntityStore &store, Entity entity, DataValue &value)> write;

    /// The format of a C++ type. `read` fills in a component that starts
    /// with its defaults, so that what a file leaves out keeps them. `write`
    /// adds to a map.
    template<typename T>
    static ComponentFormat Of(
      const std::string &name,
      const std::function<void(const DataReader &reader, T &component)> &read,
      const std::function<void(const T &component, DataValue &map)> &write)
    {
      ComponentFormat format;
      format.name = name;

      format.read = [read](const DataReader &reader, EntityStore &store, const Entity entity)
      {
        T component{};
        read(reader, component);
        store.Set(entity, component);
      };

      format.write = [write](EntityStore &store, const Entity entity, DataValue &value)
      {
        const T *component = store.Get<T>(entity);
        if (component == nullptr) { return false; }

        value = DataValue::Map();
        write(*component, value);
        return true;
      };

      return format;
    }
  };

  /// The kinds of components a scene file can hold.
  class ComponentFormats
  {
    // in the order they were added, which is the order they are written in
    std::vector<ComponentFormat> _formats;

  public:
    /// Adds a format, or replaces the one of the same name.
    void Add(const ComponentFormat &format);

    /// The format of a name, or nullptr.
    [[nodiscard]] const ComponentFormat *Find(const std::string &name) const;

    [[nodiscard]] const std::vector<ComponentFormat> &GetAll() const;

    /// Adds the formats of the components of the engine, those of the
    /// physics included.
    void AddEngineComponents();

    /// Adds the formats of the components of the physics: RigidBody,
    /// Trigger, CharacterBody, and Collider.
    void AddPhysicsComponents();
  };
} // neon

#endif //COMPONENT_FORMAT_HPP
