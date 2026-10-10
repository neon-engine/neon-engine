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

    /// The component that reading starts from: the one the entity has, so
    /// that a file that writes on top of a prefab changes only what it
    /// names, or one with the defaults of the type when the entity has
    /// none. `name` is what the component was registered under; a type
    /// that is not registered gives the defaults, and what is wrong with
    /// the values is then still read and said.
    template<typename T>
    [[nodiscard]] static T StartFrom(EntityStore &store, const std::string &name, const Entity entity)
    {
      const auto id = store.FindComponent(name);
      if (id == No_Component) { return T{}; }

      const auto *existing = static_cast<const T *>(store.GetComponentData(entity, id));
      return existing == nullptr ? T{} : *existing;
    }

    /// The format of a C++ type that is described. How it is read and
    /// written follows from the description, see TypeBuilder. A type whose
    /// description has problems reads nothing: a file that writes the
    /// component is told the problems, and the entity goes without it.
    template<typename T>
    static ComponentFormat Of()
    {
      const auto type = std::make_shared<const TypeInfo>(TypeInfo::Of<T>());

      ComponentFormat format;
      format.name = type->name;
      format.type = type;

      format.read = [type](const DataReader &reader, EntityStore &store, const Entity entity)
      {
        if (!type->problems.empty())
        {
          for (const auto &problem : type->problems)
          {
            reader.Report(reader.GetWhere() + " cannot be read, its description is wrong: " + problem);
          }
          // what was described is asked for, so that it is not reported
          // as unknown on top
          for (const auto &field : type->fields) { (void) reader.ReadValue(field.name); }
          return;
        }

        T component = StartFrom<T>(store, type->name, entity);
        ReadFields(*type, reader, &component);
        store.Set(entity, component);
      };

      format.write = [type](EntityStore &store, const Entity entity, DataValue &value)
      {
        // one that is turned off is written as well, with what it holds
        const T *component = static_cast<const T *>(store.GetComponentData(entity, store.IdOf<T>()));
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
    /// with what the entity has, or with its defaults, so that what a file
    /// leaves out keeps them. `write` adds to a map.
    template<typename T>
    static ComponentFormat Of(
      const std::string &name,
      const std::function<void(const DataReader &reader, T &component)> &read,
      const std::function<void(const T &component, DataValue &map)> &write)
    {
      ComponentFormat format;
      format.name = name;

      format.read = [read, name](const DataReader &reader, EntityStore &store, const Entity entity)
      {
        T component = StartFrom<T>(store, name, entity);
        read(reader, component);
        store.Set(entity, component);
      };

      format.write = [write](EntityStore &store, const Entity entity, DataValue &value)
      {
        // one that is turned off is written as well, with what it holds
        const T *component = static_cast<const T *>(store.GetComponentData(entity, store.IdOf<T>()));
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
    /// Trigger, CharacterBody, Collider, and Joint.
    void AddPhysicsComponents();
  };
} // neon

#endif //COMPONENT_FORMAT_HPP
