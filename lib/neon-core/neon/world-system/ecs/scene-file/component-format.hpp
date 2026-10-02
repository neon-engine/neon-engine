#ifndef COMPONENT_FORMAT_HPP
#define COMPONENT_FORMAT_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <neon/data/data-reader.hpp>
#include <neon/data/data-value.hpp>
#include <neon/reflection/field-documents.hpp>
#include <neon/reflection/type-info.hpp>
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

    /// The description of the component, when the format was made from one.
    /// It is what tells the editor and a script which fields there are.
    std::shared_ptr<const TypeInfo> type;

    /// The format of a C++ type that is described. How it is read and
    /// written follows from the description, see TypeBuilder.
    template<typename T>
    static ComponentFormat Of()
    {
      const auto type = std::make_shared<const TypeInfo>(TypeInfo::Of<T>());

      ComponentFormat format;
      format.name = type->name;
      format.type = type;

      format.read = [type](const DataReader &reader, EntityStore &store, const Entity entity)
      {
        T component{};
        ReadFields(*type, reader, &component);
        store.Set(entity, component);
      };

      format.write = [type](EntityStore &store, const Entity entity, DataValue &value)
      {
        const T *component = store.Get<T>(entity);
        if (component == nullptr) { return false; }

        const T standard{};
        value = DataValue::Map();
        WriteFields(*type, component, &standard, value);
        return true;
      };

      return format;
    }

    /// The format of a C++ type that is read and written by hand, for what
    /// a description cannot say. `read` fills in a component that starts
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

    /// The format of a component that the physics registers. A world
    /// without physics does not know the component, which is said instead
    /// of giving the entity something the store cannot hold.
    template<typename T>
    static ComponentFormat PhysicsFormat();

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
    /// Trigger, CharacterBody, Collider, and Joint.
    void AddPhysicsComponents();
  };
} // neon

#endif //COMPONENT_FORMAT_HPP
