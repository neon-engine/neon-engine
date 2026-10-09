#include "scene-file.hpp"

#include <algorithm>
#include <format>

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/prefab.hpp>

namespace neon
{
  // Helpers of SceneFile, for this file alone.
  namespace
  {
    /// What a component is written as when it keeps all of its defaults.
    const std::string all_defaults = "Default";

    // what any component takes besides its own values: whether it is turned on
    const std::string enabled_key = "enabled";

    const DataValue no_values;

    /// What the Prefab component is registered under.
    const std::string prefab_component = "Prefab";

    /// The names an entity is written with, which a child that is taken
    /// away cannot be called.
    const std::vector<std::string> entity_names = {"name", "prefab", "components", "children"};

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

    /// Whether an item of a list of children takes a child away, written
    /// as `- shade: ~`: a map of one name with nothing as its value. The
    /// name comes out in `taken`.
    bool TakesAway(const DataValue &item, std::string &taken)
    {
      if (!item.IsMap() || item.GetEntries().size() != 1) { return false; }

      const auto &[name, value] = item.GetEntries().front();
      if (!value.IsEmpty() || std::ranges::find(entity_names, name) != entity_names.end()) { return false; }

      taken = name;
      return true;
    }

    /// Whether the entity has the component, turned on or off: one that is
    /// off needs what it needs once it is turned on, as in a pool.
    bool Carries(EntityStore &store, const Entity entity, const std::string &component)
    {
      const auto id = store.FindComponent(component);
      return id != No_Component && store.GetComponentData(entity, id) != nullptr;
    }

    /// The child written under a name in what was written for an entity,
    /// or nullptr when it is not written there, as a child a prefab gave.
    const DataValue *WrittenChild(const DataValue &written, const std::string &name)
    {
      const auto *children = written.Find("children");
      if (children == nullptr || !children->IsList()) { return nullptr; }

      for (const auto &item : children->GetItems())
      {
        if (NameOf(item) == name) { return &item; }
      }
      return nullptr;
    }

    /// The names of the entities below a parent, joined for a message.
    std::string NamesBelow(EntityStore &store, const Entity parent)
    {
      std::string names;
      for (const auto child : store.GetChildren(parent))
      {
        if (!names.empty()) { names += ", "; }
        names += store.GetName(child);
      }
      return names;
    }
  }

  SceneFile::SceneFile(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::string &path,
    const std::shared_ptr<Logger> &logger)
    : _prefabs(file_system, format)
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

  bool SceneFile::ReadAhead(const std::string &path)
  {
    _read_ahead_path.clear();

    std::vector<std::string> errors;
    if (!ReadDocument(path, _read_ahead, errors))
    {
      if (!errors.empty()) { ReportProblems(errors, std::format("The scene {}", path), "the game stays where it is"); }
      return false;
    }

    _read_ahead_path = path;
    return true;
  }

  bool SceneFile::CouldNotBeRead() const
  {
    return _could_not_be_read;
  }

  const std::string &SceneFile::GetPath() const
  {
    return _path;
  }

  bool SceneFile::Read(EntityStore &store)
  {
    _logger->Info("Loading the scene from {}", _path);

    // the prefabs of the scene before are forgotten, so that a file that
    // changed is read anew
    _prefabs.Clear();
    _lacking_said.clear();
    _could_not_be_read = false;

    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    DataValue document;

    // what was read ahead of this path is not read again
    if (_read_ahead_path == _path)
    {
      document = std::move(_read_ahead);
      _read_ahead = DataValue();
      _read_ahead_path.clear();
    } else if (!ReadDocument(_path, document, errors))
    {
      _could_not_be_read = true;
      if (errors.empty()) { return false; }
    }

    // the loader is what knows where an entity came from, so the component
    // that says so is its own
    store.Register<Prefab>(prefab_component);

    if (!_could_not_be_read)
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
          ReadEntities(*entities, _path, "", store, No_Entity, false, _prefabs, errors, &warnings);
        }
      }

      reader.Finish();
    }

    // what was mended is no problem of the scene: it is said, and not counted
    ReportWarnings(warnings);

    if (errors.empty()) { return true; }

    ReportProblems(errors, std::format("The scene {}", _path), "the world holds what could be read");
    return false;
  }

  bool SceneFile::ReadDocument(
    const std::string &path,
    DataValue &document,
    std::vector<std::string> &errors) const
  {
    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      _logger->Error("The scene {} cannot be read: there is no such file, or it cannot be opened", path);
      return false;
    }

    if (std::string error; !_format->Read(path, text, document, error))
    {
      errors.push_back(error);
      return false;
    }
    return true;
  }

  void SceneFile::ReportProblems(
    const std::vector<std::string> &errors,
    const std::string &what,
    const std::string &goes_on) const
  {
    for (const auto &error : errors) { _logger->Error("{}", error); }

    // the game goes on with what could be read, and the exit code says that
    // not everything could
    const std::size_t count = errors.size();
    const std::string problems = count == 1 ? "problem" : "problems";
    _logger->Error("{} has {} {}, {}", what, count, problems, goes_on);
  }

  Entity SceneFile::Spawn(
    EntityStore &store,
    const std::string &path,
    const Entity parent,
    const DataValue &overrides)
  {
    store.Register<Prefab>(prefab_component);

    // what is spawned is read as an entity of a scene would be: the prefab
    // first, then the overrides on top, as the components written next to
    // `prefab:` in a file
    auto written = DataValue::Map();
    if (!overrides.IsEmpty()) { written.Set("components", overrides); }

    std::vector<std::string> errors;
    const DataReader reader(written, std::format("spawn of {}", path), "the spawned entity", errors);

    const Entity entity = store.CreateEntity("", parent);
    const bool placed = PlacePrefab(path, DataValue::Text(path), reader, store, entity, false, _prefabs);
    if (!placed)
    {
      store.DestroyEntity(entity);
      ReportProblems(errors, std::format("The prefab {}", path), "nothing is spawned");
      return No_Entity;
    }

    ReadComponents(reader, written, store, entity);
    reader.Finish();

    // what a prefab lacks is mended for every spawn, and said for its
    // first, not for every nail
    const std::size_t errors_before = errors.size();
    std::vector<std::string> warnings;
    CheckNeeds(store, entity, written, reader.GetDocument(), reader.GetWhere(), errors, warnings);
    if (std::ranges::find(_lacking_said, path) != _lacking_said.end())
    {
      errors.resize(errors_before);
    } else if (errors.size() > errors_before || !warnings.empty())
    {
      _lacking_said.push_back(path);
      ReportWarnings(warnings);
    }

    if (!errors.empty())
    {
      ReportProblems(errors, std::format("The spawn of {}", path), "the entity holds what could be read");
    }
    return entity;
  }

  void SceneFile::ReadEntities(
    const DataValue &list,
    const std::string &document,
    const std::string &of,
    EntityStore &store,
    const Entity parent,
    const bool onto_existing,
    PrefabFiles &prefabs,
    std::vector<std::string> &errors,
    std::vector<std::string> *warnings) const
  {
    std::vector<std::string> names;
    std::size_t number = 1;
    for (const auto &item : list.GetItems())
    {
      const auto label = of.empty() ? std::format("entity {}", number) : std::format("child {} of {}", number, of);
      number++;

      std::string taken;
      const bool takes_away = TakesAway(item, taken);

      // a name twice in one list is a mistake even where a name that is
      // there already is read onto, since the second would change the first
      if (const auto name = takes_away ? taken : NameOf(item); !name.empty())
      {
        if (std::ranges::find(names, name) != names.end())
        {
          const DataReader reader(item, document, "entity '" + name + "'", errors);
          reader.Report(item, std::format("entity '{}' shares its name with another entity next to it", name));
          continue;
        }
        names.push_back(name);
      }

      if (takes_away)
      {
        TakeAwayChild(item, taken, document, of, store, parent, onto_existing, errors);
        continue;
      }

      const Entity entity = ReadEntity(item, document, label, store, parent, onto_existing, prefabs, errors);

      // An entity at the top of a scene is whole once it is read: its
      // prefab, what is written on top, and its children, those a prefab
      // gave included. Only then is it known what it lacks.
      if (warnings != nullptr && entity != No_Entity)
      {
        const std::string name = NameOf(item);
        const std::string where = name.empty() ? label : "entity '" + name + "'";
        CheckNeeds(store, entity, item, document, where, errors, *warnings);
      }
    }
  }

  void SceneFile::TakeAwayChild(
    const DataValue &item,
    const std::string &name,
    const std::string &document,
    const std::string &of,
    EntityStore &store,
    const Entity parent,
    const bool onto_existing,
    std::vector<std::string> &errors) const
  {
    const std::string where = of.empty() ? std::format("entity '{}'", name) : std::format("child '{}' of {}", name, of);
    const DataReader reader(item, document, where, errors);

    // only what a prefab gave can be taken away: anywhere else there is
    // nothing of that name yet, and at the top of a scene an entity of a
    // scene before may stay, which a scene must not take away by name
    if (!onto_existing)
    {
      reader.Report(item, std::format("{} is taken away, and only a child of a prefab can be", where));
      return;
    }

    for (const auto child : store.GetChildren(parent))
    {
      if (store.GetName(child) != name) { continue; }

      store.DestroyEntity(child);
      return;
    }

    // the same as a component that is not known, with what is known
    const std::string known = NamesBelow(store, parent);
    reader.Report(item, known.empty()
                    ? std::format("{} is not known. {} has no children", where, of)
                    : std::format("{} is not known. Known are: {}", where, known));
  }

  Entity SceneFile::ReadEntity(
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
          return No_Entity;
        }

        entity = sibling;
        break;
      }
    }

    if (entity == No_Entity) { entity = store.CreateEntity(name, parent); }

    ReadOnto(reader, value, store, entity, onto_existing, prefabs);
    reader.Finish();
    return entity;
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

    // `enabled: false` turns the component off: it keeps what was written
    // for it, and is not there until it is turned on, see
    // EntityStore::SetEnabled(). Any component takes it.
    if (bool enabled = true; component_reader.Read(enabled_key, enabled))
    {
      if (const auto id = store.FindComponent(name); id != No_Component) { store.SetEnabled(entity, id, enabled); }
    }
    component_reader.Finish();
  }

  void SceneFile::CheckNeeds(
    EntityStore &store,
    const Entity entity,
    const DataValue &written,
    const std::string &document,
    const std::string &where,
    std::vector<std::string> &errors,
    std::vector<std::string> &warnings) const
  {
    const DataReader problems(written, document, where, errors);
    const DataReader mended(written, document, where, warnings);

    // What is drawn without a Transform would be drawn nowhere, which is
    // never what is meant: it is given one of its defaults, at its parent's
    // place, and the warning says where to write it. One is enough for all.
    const bool can_place = store.FindComponent("Transform") != No_Component;
    const auto placed = [&](const std::string &component)
    {
      if (!Carries(store, entity, component) || Carries(store, entity, "Transform")) { return; }

      if (!can_place)
      {
        problems.Report(written, std::format("{} has a {} and no Transform, so it is drawn nowhere", where, component));
        return;
      }

      store.Set(entity, Transform{});
      const std::string at = store.GetParent(entity) == No_Entity ? "at the origin" : "where its parent is";
      mended.Report(written, std::format("{} has a {} and no Transform; one was added {}", where, component, at));
    };

    // what RenderSubmission asks for
    placed("Renderable");
    placed("Camera");
    placed("Light");

    // A Renderable of its defaults has no shader, which the renderer
    // refuses, so one that is missing is a mistake of the recipe, as a
    // name that is misspelled is: the entity is kept as it is written.
    const auto lacks = [&](const std::string &component, const std::string &follows)
    {
      if (!Carries(store, entity, component) || Carries(store, entity, "Renderable")) { return; }

      problems.Report(written, std::format("{} has a {} and no Renderable, so {}", where, component, follows));
    };

    // what RopeDrawing and GeometryBuilding ask for
    lacks("Rope", "the rope is drawn nowhere");

    // a Geometry without a Renderable is the shape of a Collider that is
    // not seen, such as an invisible wall; without either it does nothing
    if (!Carries(store, entity, "Collider")) { lacks("Geometry", "its shape is drawn nowhere"); }

    std::size_t number = 1;
    for (const auto child : store.GetChildren(entity))
    {
      const std::string name = store.GetName(child);
      const std::string child_where = name.empty()
                                        ? std::format("child {} of {}", number, where)
                                        : std::format("child '{}' of {}", name, where);
      number++;

      // a child that is not written here, one a prefab gave, is said at
      // the line of the entity it is below
      const auto *child_written = name.empty() ? nullptr : WrittenChild(written, name);
      CheckNeeds(
        store, child, child_written != nullptr ? *child_written : written, document, child_where, errors, warnings);
    }
  }

  void SceneFile::ReportWarnings(const std::vector<std::string> &warnings) const
  {
    for (const auto &warning : warnings) { _logger->Warn("{}", warning); }
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

  void SceneFile::ChildrenOfPrefab(
    const std::string &path,
    std::vector<std::string> &names,
    std::vector<std::string> &seen)
  {
    // a loop was refused when the prefab was placed; here it is only not
    // followed
    if (std::ranges::find(seen, path) != seen.end()) { return; }
    seen.push_back(path);

    // a prefab that cannot be read was said where it was placed, and gave
    // no children
    std::vector<std::string> errors;
    const auto *prefab = _prefabs.Find(path, errors);
    if (prefab == nullptr) { return; }

    const auto &entity = prefab->GetEntity();

    // the children of the prefab it starts from come first, as they are
    // placed first
    if (const auto *base = entity.Find("prefab"); base != nullptr)
    {
      if (std::string base_path; base->GetText(base_path)) { ChildrenOfPrefab(base_path, names, seen); }
    }

    const auto *children = entity.Find("children");
    if (children == nullptr || !children->IsList()) { return; }

    for (const auto &item : children->GetItems())
    {
      if (std::string taken; TakesAway(item, taken))
      {
        std::erase(names, taken);
        continue;
      }

      const auto name = NameOf(item);
      if (!name.empty() && std::ranges::find(names, name) == names.end()) { names.push_back(name); }
    }
  }

  DataValue SceneFile::WriteEntity(EntityStore &store, const Entity entity)
  {
    auto value = DataValue::Map();

    if (const auto name = store.GetName(entity); !name.empty())
    {
      value.Set("name", DataValue::Text(name));
    }

    // the prefab it came from is written next to its name, as it is read
    std::string prefab_path;
    if (const auto id = store.FindComponent(prefab_component); id != No_Component)
    {
      if (const auto *prefab = static_cast<const Prefab *>(store.GetComponent(entity, id)); prefab != nullptr)
      {
        prefab_path = prefab->path;
        value.Set("prefab", DataValue::Text(prefab_path));
      }
    }

    auto components = DataValue::Map();
    for (const auto &format : _component_formats.GetAll())
    {
      // a format of a component that no one registered has nothing to write
      if (store.FindComponent(format.name) == No_Component) { continue; }

      if (DataValue component; format.write(store, entity, component))
      {
        // A component that is turned off says so. One that is on says
        // nothing of it, which is what it is unless it is written.
        const bool is_off = !store.IsEnabled(entity, store.FindComponent(format.name));
        if (is_off)
        {
          if (!component.IsMap()) { component = DataValue::Map(); }
          component.Set(enabled_key, DataValue::Bool(false));
        }

        if (component.IsMap() && component.GetEntries().empty()) { component = DataValue::Text(all_defaults); }
        components.Set(format.name, component);
      }
    }
    value.Set("components", components);

    auto children = DataValue::List();

    // a child the prefab gave that the entity does not have is written as
    // taken away, so that loading the file gives the same world
    if (!prefab_path.empty())
    {
      std::vector<std::string> given;
      std::vector<std::string> seen;
      ChildrenOfPrefab(prefab_path, given, seen);

      for (const auto &given_name : given)
      {
        const bool has = std::ranges::any_of(store.GetChildren(entity), [&](const Entity child)
        {
          return store.GetName(child) == given_name;
        });
        if (has) { continue; }

        auto taken = DataValue::Map();
        taken.Set(given_name, DataValue{});
        children.Add(taken);
      }
    }

    for (const auto child : store.GetChildren(entity))
    {
      children.Add(WriteEntity(store, child));
    }
    if (!children.GetItems().empty()) { value.Set("children", children); }

    return value;
  }

  bool SceneFile::Save(EntityStore &store, const std::string &name, const std::string &path)
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
