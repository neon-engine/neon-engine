#include "scene-file.hpp"

#include <format>
#include <stdexcept>

namespace neon
{
  namespace
  {
    /// What a component is written as when it keeps all of its defaults.
    const std::string all_defaults = "Default";

    const DataValue no_values;
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

  void SceneFile::Populate(EntityStore &store)
  {
    _logger->Info("Loading the scene from {}", _path);

    std::string text;
    if (!_file_system->ReadText(_path, text))
    {
      throw std::runtime_error("The scene " + _path + " cannot be read");
    }

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
          std::size_t number = 1;
          for (const auto &entity : entities->GetItems())
          {
            ReadEntity(entity, std::format("entity {}", number), store, No_Entity, errors);
            number++;
          }
        }
      }

      reader.Finish();
    }

    if (errors.empty()) { return; }

    for (const auto &error : errors) { _logger->Error("{}", error); }

    throw std::runtime_error(std::format(
      "The scene {} has {} {}, the first is: {}",
      _path, errors.size(), errors.size() == 1 ? "problem" : "problems", errors.front()));
  }

  void SceneFile::ReadEntity(
    const DataValue &value,
    const std::string &label,
    EntityStore &store,
    const Entity parent,
    std::vector<std::string> &errors) const
  {
    std::string name;
    if (const auto *written = value.Find("name"); written != nullptr)
    {
      (void) written->GetText(name);
    }

    // an entity is called by its name in messages, and by its place in the
    // file when it has none
    const std::string where = name.empty() ? label : "entity '" + name + "'";
    const DataReader reader(value, _path, where, errors);

    reader.Read("name", name);

    if (name.find('/') != std::string::npos)
    {
      reader.Report(value, std::format("the name of {} holds a '/', which separates names in a path", where));
      name.clear();
    }

    if (!name.empty())
    {
      for (const auto sibling : store.GetChildren(parent))
      {
        if (store.GetName(sibling) != name) { continue; }

        reader.Report(value, std::format("{} shares its name with another entity next to it", where));
        return;
      }
    }

    const auto entity = store.CreateEntity(name, parent);

    bool has_components = false;
    const auto components = reader.ReadMap("components", has_components);

    if (const auto *written = value.Find("components"); written != nullptr && written->IsMap())
    {
      for (const auto &[component_name, component] : written->GetEntries())
      {
        // asks for the name, so that the reader does not take it as unknown
        (void) components.ReadValue(component_name);

        const auto *format = _component_formats.Find(component_name);
        if (format == nullptr)
        {
          std::string known;
          for (const auto &known_format : _component_formats.GetAll())
          {
            if (!known.empty()) { known += ", "; }
            known += known_format.name;
          }

          components.Report(component, std::format(
                              "component '{}' of {} is not known. Known are: {}",
                              component_name, where, known));
          continue;
        }

        // `Default` stands for a component with all of its defaults
        const DataValue *values = &component;
        if (std::string text; component.GetText(text))
        {
          if (text != all_defaults)
          {
            components.Report(component, std::format(
                                "{} of {} is '{}'. Write its values, or {} to keep all of its defaults",
                                component_name, where, text, all_defaults));
            continue;
          }
          values = &no_values;
        }

        const DataReader component_reader(
          *values,
          _path,
          std::format("{} of {}", component_name, where),
          errors);

        format->read(component_reader, store, entity);
        component_reader.Finish();
      }
    }

    components.Finish();

    if (const auto *children = reader.ReadValue("children"); children != nullptr)
    {
      if (!children->IsList())
      {
        reader.Report(*children, std::format(
                        "'children' of {} is {}, where a list was expected",
                        where, DataValue::Describe(children->GetKind())));
      } else
      {
        std::size_t number = 1;
        for (const auto &child : children->GetItems())
        {
          ReadEntity(child, std::format("child {} of {}", number, where), store, entity, errors);
          number++;
        }
      }
    }

    reader.Finish();
  }

  DataValue SceneFile::WriteEntity(EntityStore &store, const Entity entity) const
  {
    auto value = DataValue::Map();

    if (const auto name = store.GetName(entity); !name.empty())
    {
      value.Set("name", DataValue::Text(name));
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
