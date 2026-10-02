#include "scene-file.hpp"

#include <algorithm>
#include <format>

#include <neon/world-system/ecs/components/prefab.hpp>

namespace neon
{
  // Helpers of SceneFile, for this file alone.
  namespace
  {
    /// What a component is written as when it keeps all of its defaults.
    const std::string all_defaults = "Default";

    const DataValue no_values;

    /// What the Prefab component is registered under.
    const std::string prefab_component = "Prefab";

    /// The name written for an entity, or empty.
    std::string NameOf(const DataValue &entity)
    {
      std::string name;
      if (const auto *written = entity.Find("name"); written != nullptr)
      {
        (void) written->GetText(name);
      }
      return name;
    }
  }

  SceneFile::SceneFile(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::string &path,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _path = path;
    _logger = logger;

    _component_formats.AddEngineComponents();
  }

  ComponentFormats &SceneFile::GetComponentFormats()
  {
    return _component_formats;
  }

  bool SceneFile::Populate(EntityStore &store)
  {
    return Read(store);
  }

  bool SceneFile::Load(EntityStore &store, const std::string &path)
  {
    _path = path;
    return Read(store);
  }

  const std::string &SceneFile::GetPath() const
  {
    return _path;
  }

  bool SceneFile::Read(EntityStore &store)
  {
    _logger->Info("Loading the scene from {}", _path);

    std::string text;
    if (!_file_system->ReadText(_path, text))
    {
      _logger->Error("The scene {} cannot be read, the world starts empty", _path);
      return false;
    }

    // the loader is what knows where an entity came from, so the component
    // that says so is its own
    store.Register<Prefab>(prefab_component);

    std::vector<std::string> errors;
    DataValue document;

    if (std::string error; !_format->Read(_path, text, document, error))
    {
      errors.push_back(error);
    } else
    {
      const DataReader reader(document, _path, "the scene", errors);

      std::string name;
      reader.Read("scene", name);

      if (float written = 0.0f; reader.Read("version", written) && written > static_cast<float>(version))
      {
        reader.Report(*document.Find("version"), std::format(
                        "the scene has version {}, and this engine reads up to version {}",
                        written, version));
      }

      if (const auto *entities = reader.ReadValue("entities"); entities != nullptr)
      {
        if (!entities->IsList())
        {
          reader.Report(*entities, std::format(
                          "'entities' of the scene is {}, where a list was expected",
                          DataValue::Describe(entities->GetKind())));
        } else
        {
          // the prefabs are kept for this load alone, so that a file that
          // changed is read anew the next time
          PrefabFiles prefabs(_file_system, _format);
          ReadEntities(*entities, _path, "", store, No_Entity, false, prefabs, errors);
        }
      }

      reader.Finish();
    }

    if (errors.empty()) { return true; }

    for (const auto &error : errors) { _logger->Error("{}", error); }

    // the game goes on with what could be read, and the exit code says that
    // not everything could
    const std::size_t count = errors.size();
    const std::string problems = count == 1 ? "problem" : "problems";
    _logger->Error("The scene {} has {} {}, the world holds what could be read", _path, count, problems);
    return false;
  }

  void SceneFile::ReadEntities(
    const DataValue &list,
    const std::string &document,
    const std::string &of,
    EntityStore &store,
    const Entity parent,
    const bool onto_existing,
    PrefabFiles &prefabs,
    std::vector<std::string> &errors) const
  {
    std::vector<std::string> names;
    std::size_t number = 1;
    for (const auto &item : list.GetItems())
    {
      const auto label = of.empty() ? std::format("entity {}", number) : std::format("child {} of {}", number, of);
      number++;

      // a name twice in one list is a mistake even where a name that is
      // there already is read onto, since the second would change the first
      if (const auto name = NameOf(item); !name.empty())
      {
        if (std::ranges::find(names, name) != names.end())
        {
          const DataReader reader(item, document, "entity '" + name + "'", errors);
          reader.Report(item, std::format("entity '{}' shares its name with another entity next to it", name));
          continue;
        }
        names.push_back(name);
      }

      ReadEntity(item, document, label, store, parent, onto_existing, prefabs, errors);
    }
  }

  void SceneFile::ReadEntity(
    const DataValue &value,
    const std::string &document,
    const std::string &label,
    EntityStore &store,
    const Entity parent,
    const bool onto_existing,
    PrefabFiles &prefabs,
    std::vector<std::string> &errors) const
  {
    std::string name = NameOf(value);

    // an entity is called by its name in messages, and by its place in the
    // file when it has none
    const std::string where = name.empty() ? label : "entity '" + name + "'";
    const DataReader reader(value, document, where, errors);

    reader.Read("name", name);

    if (name.find('/') != std::string::npos)
    {
      reader.Report(value, std::format("the name of {} holds a '/', which separates names in a path", where));
      name.clear();
    }

    // an entity of the name that is there already is one a prefab made,
    // which what is written here changes; anywhere else the name is taken
    Entity entity = No_Entity;
    if (!name.empty())
    {
      for (const auto sibling : store.GetChildren(parent))
      {
        if (store.GetName(sibling) != name) { continue; }

        if (!onto_existing)
        {
          reader.Report(value, std::format("{} shares its name with another entity next to it", where));
          return;
        }

        entity = sibling;
        break;
      }
    }

    if (entity == No_Entity) { entity = store.CreateEntity(name, parent); }

    ReadOnto(reader, value, store, entity, onto_existing, prefabs);
    reader.Finish();
  }

  void SceneFile::ReadOnto(
    const DataReader &reader,
    const DataValue &value,
    EntityStore &store,
    const Entity entity,
    const bool onto_existing,
    PrefabFiles &prefabs) const
  {
    // the prefab comes first, so that what is written here lands on top
    bool placed = false;
    if (std::string path; reader.Read("prefab", path))
    {
      placed = PlacePrefab(path, *reader.ReadValue("prefab"), reader, store, entity, onto_existing, prefabs);
    }

    ReadComponents(reader, value, store, entity);

    if (const auto *children = reader.ReadValue("children"); children != nullptr)
    {
      if (!children->IsList())
      {
        reader.Report(*children, std::format(
                        "'children' of {} is {}, where a list was expected",
                        reader.GetWhere(), DataValue::Describe(children->GetKind())));
      } else
      {
        // below a prefab, a child of a name the prefab gave is that child
        ReadEntities(
          *children,
          reader.GetDocument(),
          reader.GetWhere(),
          store,
          entity,
          onto_existing || placed,
          prefabs,
          reader.GetErrors());
      }
    }
  }

  bool SceneFile::PlacePrefab(
    const std::string &path,
    const DataValue &written,
    const DataReader &reader,
    EntityStore &store,
    const Entity entity,
    const bool onto_existing,
    PrefabFiles &prefabs) const
  {
    const auto &where = reader.GetWhere();
    auto &errors = reader.GetErrors();

    if (prefabs.IsPlacing(path))
    {
      reader.Report(written, std::format(
                      "'prefab' of {} places {} within itself: {}",
                      where, path, prefabs.DescribeLoop(path, reader.GetDocument(), written.GetLine())));
      return false;
    }

    const auto *prefab = prefabs.Find(path, errors);
    if (prefab == nullptr)
    {
      reader.Report(written, std::format("'prefab' of {} is {}, which cannot be read", where, path));
      return false;
    }

    // what is wrong inside a prefab is said the first time it is placed,
    // not once for every wall of a room
    std::vector<std::string> said_already;
    auto &problems = prefabs.WasPlaced(path) ? said_already : errors;

    prefabs.BeginPlacing(path, reader.GetDocument(), written.GetLine());

    const DataReader prefab_reader(prefab->GetEntity(), path, where, problems);
    ReadOnto(prefab_reader, prefab->GetEntity(), store, entity, onto_existing, prefabs);
    prefab_reader.Finish();

    prefabs.EndPlacing();

    // set last, so that a prefab placed by the prefab leaves the outer one
    // as what the entity came from
    store.Set(entity, Prefab{.path = path});
    return true;
  }

  void SceneFile::ReadComponents(
    const DataReader &reader,
    const DataValue &value,
    EntityStore &store,
    const Entity entity) const
  {
    bool has_components = false;
    const auto components = reader.ReadMap("components", has_components);

    if (const auto *written = value.Find("components"); written != nullptr && written->IsMap())
    {
      for (const auto &[component_name, component] : written->GetEntries())
      {
        // asks for the name, so that the reader does not take it as unknown
        (void) components.ReadValue(component_name);

        ReadComponent(components, reader.GetWhere(), component_name, component, store, entity);
      }
    }

    components.Finish();
  }

  void SceneFile::ReadComponent(
    const DataReader &components,
    const std::string &where,
    const std::string &name,
    const DataValue &component,
    EntityStore &store,
    const Entity entity) const
  {
    const auto *format = _component_formats.Find(name);
    if (format == nullptr)
    {
      components.Report(component, std::format(
                          "component '{}' of {} is not known. Known are: {}",
                          name, where, KnownComponents()));
      return;
    }

    // nothing, written as `~`, takes the component away: one a prefab gave
    // the entity that this entity does without
    if (component.IsEmpty())
    {
      if (const auto id = store.FindComponent(name); id != No_Component && store.HasComponent(entity, id))
      {
        store.RemoveComponent(entity, id);
      }
      return;
    }

    // `Default` stands for a component with all of its defaults
    const DataValue *values = &component;
    if (std::string text; component.GetText(text))
    {
      if (text != all_defaults)
      {
        components.Report(component, std::format(
                            "{} of {} is '{}'. Write its values, or {} to keep all of its defaults",
                            name, where, text, all_defaults));
        return;
      }
      values = &no_values;
    }

    const DataReader component_reader(
      *values,
      components.GetDocument(),
      std::format("{} of {}", name, where),
      components.GetErrors());

    format->read(component_reader, store, entity);
    component_reader.Finish();
  }

  std::string SceneFile::KnownComponents() const
  {
    std::string known;
    for (const auto &format : _component_formats.GetAll())
    {
      if (!known.empty()) { known += ", "; }
      known += format.name;
    }
    return known;
  }

  DataValue SceneFile::WriteEntity(EntityStore &store, const Entity entity) const
  {
    auto value = DataValue::Map();

    if (const auto name = store.GetName(entity); !name.empty())
    {
      value.Set("name", DataValue::Text(name));
    }

    // the prefab it came from is written next to its name, as it is read
    if (const auto id = store.FindComponent(prefab_component); id != No_Component)
    {
      if (const auto *prefab = static_cast<const Prefab *>(store.GetComponent(entity, id)); prefab != nullptr)
      {
        value.Set("prefab", DataValue::Text(prefab->path));
      }
    }

    auto components = DataValue::Map();
    for (const auto &format : _component_formats.GetAll())
    {
      // a format of a component that no one registered has nothing to write
      if (store.FindComponent(format.name) == No_Component) { continue; }

      if (DataValue component; format.write(store, entity, component))
      {
        if (component.IsMap() && component.GetEntries().empty()) { component = DataValue::Text(all_defaults); }
        components.Set(format.name, component);
      }
    }
    value.Set("components", components);

    auto children = DataValue::List();
    for (const auto child : store.GetChildren(entity))
    {
      children.Add(WriteEntity(store, child));
    }
    if (!children.GetItems().empty()) { value.Set("children", children); }

    return value;
  }

  bool SceneFile::Save(EntityStore &store, const std::string &name, const std::string &path) const
  {
    auto document = DataValue::Map();
    document.Set("scene", DataValue::Text(name));
    document.Set("version", DataValue::Number(version));

    auto entities = DataValue::List();
    for (const auto entity : store.GetChildren(No_Entity))
    {
      entities.Add(WriteEntity(store, entity));
    }
    document.Set("entities", entities);

    if (!_file_system->WriteText(path, _format->Write(document)))
    {
      _logger->Error("The scene could not be written to {}", path);
      return false;
    }

    _logger->Info("Saved the scene to {}", path);
    return true;
  }
} // neon
