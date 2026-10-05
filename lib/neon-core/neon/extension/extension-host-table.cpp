#include "extension-host-table.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <vector>

#include <neon/world-system/ecs/systems/pool-system.hpp>
#include <neon/filesystem/file-system.hpp>
#include <neon/reflection/field-numbers.hpp>
#include <neon/reflection/field-text.hpp>
#include <neon/scripting/script-component-layout.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>

namespace neon
{
  // What an extension calls through its NeonExtensionHost, for this file
  // alone. Each is handed the extension as its context.
  namespace
  {
    LoadedExtension &of(void *context)
    {
      return *static_cast<LoadedExtension *>(context);
    }

    void log(void *context, const NeonLogLevel level, const char *message)
    {
      const auto &extension = of(context);
      const std::string text = message == nullptr ? "" : message;

      switch (level)
      {
        case NEON_LOG_TRACE: extension.logger->Trace("{}", text);
          break;
        case NEON_LOG_DEBUG: extension.logger->Debug("{}", text);
          break;
        case NEON_LOG_WARN: extension.logger->Warn("{}", text);
          break;
        case NEON_LOG_ERROR: extension.logger->Error("{}", text);
          break;
        case NEON_LOG_CRITICAL: extension.logger->Critical("{}", text);
          break;
        case NEON_LOG_INFO:
        default: extension.logger->Info("{}", text);
          break;
      }
    }

    /// The store, or nullptr after saying that there is none now.
    EntityStore *store_for(LoadedExtension &extension, const std::string &function)
    {
      if (extension.store == nullptr)
      {
        extension.logger->Error(
          "{} was called while there is no world. The store is reached from 'start' on, until the world is cleaned up",
          function);
      }
      return extension.store;
    }

    /// The kind of the engine for a kind of the C file. Returns false for
    /// one that is not known.
    bool kind_of(const NeonFieldKind described, FieldKind &kind)
    {
      switch (described)
      {
        case NEON_FIELD_BOOLEAN: kind = FieldKind::Boolean;
          return true;
        case NEON_FIELD_INTEGER: kind = FieldKind::Integer;
          return true;
        case NEON_FIELD_FLOAT: kind = FieldKind::Float;
          return true;
        case NEON_FIELD_DOUBLE: kind = FieldKind::Double;
          return true;
        case NEON_FIELD_VECTOR2: kind = FieldKind::Vector2;
          return true;
        case NEON_FIELD_VECTOR3: kind = FieldKind::Vector3;
          return true;
        case NEON_FIELD_VECTOR4: kind = FieldKind::Vector4;
          return true;
        case NEON_FIELD_COLOR: kind = FieldKind::Color;
          return true;
        case NEON_FIELD_QUATERNION: kind = FieldKind::Quaternion;
          return true;
        case NEON_FIELD_BYTE: kind = FieldKind::Byte;
          return true;
        case NEON_FIELD_SHORT: kind = FieldKind::Short;
          return true;
        case NEON_FIELD_UNSIGNED_SHORT: kind = FieldKind::UnsignedShort;
          return true;
        case NEON_FIELD_UNSIGNED_INTEGER: kind = FieldKind::UnsignedInteger;
          return true;
        case NEON_FIELD_LONG: kind = FieldKind::Long;
          return true;
        case NEON_FIELD_UNSIGNED_LONG: kind = FieldKind::UnsignedLong;
          return true;
        case NEON_FIELD_INTEGER_VECTOR2: kind = FieldKind::IntegerVector2;
          return true;
        case NEON_FIELD_INTEGER_VECTOR3: kind = FieldKind::IntegerVector3;
          return true;
        default: return false;
      }
    }

    /// What a field of a kind starts with, from the numbers of its
    /// description. Returns false for a kind that is not known.
    bool standard_of(const NeonFieldDescription &field, FieldKind &kind, FieldValue &value)
    {
      return kind_of(field.kind, kind) && FromNumbers(kind, field.standard, value);
    }

    NeonComponent register_component(void *context, const NeonComponentDescription *description)
    {
      auto &extension = of(context);

      if (extension.store == nullptr || extension.formats == nullptr)
      {
        extension.logger->Error(
          "register_component was called outside of 'register_components' of the extension, which is the one "
          "place components are registered");
        return No_Component;
      }
      if (description == nullptr || description->name == nullptr || description->name[0] == '\0')
      {
        extension.logger->Error("A component was registered without a name");
        return No_Component;
      }

      const std::string name = description->name;
      if (description->field_count > 0 && description->fields == nullptr)
      {
        extension.logger->Error("The component '{}' says it has fields and describes none", name);
        return No_Component;
      }
      if (extension.store->FindComponent(name) != No_Component)
      {
        extension.logger->Error("The component '{}' cannot be registered: there is a component of that name", name);
        return No_Component;
      }

      std::vector<ScriptField> fields;
      for (std::uint64_t i = 0; i < description->field_count; i++)
      {
        const NeonFieldDescription &described = description->fields[i];

        ScriptField field;
        field.name = described.name == nullptr ? "" : described.name;
        field.description = described.description == nullptr ? "" : described.description;
        if (!standard_of(described, field.kind, field.standard))
        {
          const int kind = described.kind;
          extension.logger->Error(
            "The field '{}' of the component '{}' is of kind {}, which this application does not know",
            field.name,
            name,
            kind);
          return No_Component;
        }
        fields.push_back(field);
      }

      // The fields lie as a script's do, one after the other, each at the
      // alignment of what it holds, which is how a compiler lays out the
      // struct of the extension. That the two agree is checked, since a
      // difference would be read as wrong values and nothing else.
      const ScriptComponentLayout layout(
        name,
        description->description == nullptr ? "" : description->description,
        fields);
      if (!layout.GetProblem().empty())
      {
        extension.logger->Error("The component '{}' cannot be registered: {}", name, layout.GetProblem());
        return No_Component;
      }

      for (std::size_t i = 0; i < fields.size(); i++)
      {
        const std::uint64_t given = description->fields[i].offset;
        const std::size_t expected = layout.GetOffset(i);
        if (given != expected)
        {
          extension.logger->Error(
            "The component '{}' cannot be registered: its field '{}' lies at byte {} of the struct of the "
            "extension, and at byte {} by its description. The struct holds the described fields alone, in the "
            "order they are described",
            name,
            fields[i].name,
            given,
            expected);
          return No_Component;
        }
      }

      // a struct without fields has the size the language gives it
      const std::uint64_t given_size = description->size;
      const std::size_t expected_size = layout.GetSize();
      if (given_size != expected_size && !(fields.empty() && given_size == 0))
      {
        extension.logger->Error(
          "The component '{}' cannot be registered: the struct of the extension has {} bytes, and {} by its "
          "description. The struct holds the described fields alone, in the order they are described",
          name,
          given_size,
          expected_size);
        return No_Component;
      }

      const ComponentId id = extension.store->RegisterComponent(layout.GetComponentInfo());
      extension.formats->Add(layout.GetComponentFormat());

      extension.logger->Info("Registered the component '{}'", name);
      return id;
    }

    NeonComponent find_component(void *context, const char *name)
    {
      auto *store = store_for(of(context), "find_component");
      if (store == nullptr || name == nullptr) { return No_Component; }
      return store->FindComponent(name);
    }

    NeonEntity create_entity(void *context, const char *name, const NeonEntity parent)
    {
      auto *store = store_for(of(context), "create_entity");
      if (store == nullptr) { return No_Entity; }
      return store->CreateEntity(name == nullptr ? "" : name, parent);
    }

    void destroy_entity(void *context, const NeonEntity entity)
    {
      if (auto *store = store_for(of(context), "destroy_entity")) { store->DestroyEntity(entity); }
    }

    int is_alive(void *context, const NeonEntity entity)
    {
      auto *store = store_for(of(context), "is_alive");
      return store != nullptr && store->IsAlive(entity) ? 1 : 0;
    }

    NeonEntity find_entity(void *context, const char *path)
    {
      auto *store = store_for(of(context), "find_entity");
      if (store == nullptr || path == nullptr) { return No_Entity; }
      return store->FindEntity(path);
    }

    void set_parent(void *context, const NeonEntity entity, const NeonEntity parent)
    {
      if (auto *store = store_for(of(context), "set_parent")) { store->SetParent(entity, parent); }
    }

    NeonEntity get_parent(void *context, const NeonEntity entity)
    {
      auto *store = store_for(of(context), "get_parent");
      return store == nullptr ? No_Entity : store->GetParent(entity);
    }

    /// Whether the entity and the component can be worked with, after
    /// saying what is wrong when they cannot.
    bool usable(
      LoadedExtension &extension,
      EntityStore &store,
      const std::string &function,
      const NeonEntity entity,
      const NeonComponent component)
    {
      if (component == No_Component)
      {
        extension.logger->Error("{} was called with no component", function);
        return false;
      }
      if (!store.IsAlive(entity))
      {
        extension.logger->Error("{} was called with an entity that is not alive", function);
        return false;
      }
      return true;
    }

    void set_component(void *context, const NeonEntity entity, const NeonComponent component, const void *value)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "set_component");
      if (store == nullptr || !usable(extension, *store, "set_component", entity, component)) { return; }

      if (value == nullptr)
      {
        extension.logger->Error("set_component was called without a value to copy");
        return;
      }

      store->SetComponent(entity, component, value);
    }

    void *get_component(void *context, const NeonEntity entity, const NeonComponent component)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "get_component");
      if (store == nullptr || !usable(extension, *store, "get_component", entity, component)) { return nullptr; }
      return store->GetComponent(entity, component);
    }

    int has_component(void *context, const NeonEntity entity, const NeonComponent component)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "has_component");
      if (store == nullptr || !usable(extension, *store, "has_component", entity, component)) { return 0; }
      return store->HasComponent(entity, component) ? 1 : 0;
    }

    void remove_component(void *context, const NeonEntity entity, const NeonComponent component)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "remove_component");
      if (store == nullptr || !usable(extension, *store, "remove_component", entity, component)) { return; }
      store->RemoveComponent(entity, component);
    }

    int set_component_enabled(
      void *context,
      const NeonEntity entity,
      const NeonComponent component,
      const int enabled)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "set_component_enabled");
      if (store == nullptr || !usable(extension, *store, "set_component_enabled", entity, component)) { return 0; }
      if (store->GetComponentData(entity, component) == nullptr) { return 0; }

      store->SetEnabled(entity, component, enabled != 0);
      return 1;
    }

    int is_component_enabled(void *context, const NeonEntity entity, const NeonComponent component)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "is_component_enabled");
      if (store == nullptr || !usable(extension, *store, "is_component_enabled", entity, component)) { return 0; }
      return store->IsEnabled(entity, component) ? 1 : 0;
    }

    std::uint64_t pool_add(
      void *context,
      const NeonEntity pool,
      const char *kind,
      const NeonEntity *instances,
      const std::uint64_t count)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "pool_add");
      if (store == nullptr || kind == nullptr || (instances == nullptr && count > 0)) { return 0; }

      const std::size_t before = PoolSystem::GetCount(*store, pool, kind);
      const std::vector<Entity> given(instances, instances + count);
      if (!PoolSystem::Add(*store, pool, kind, given, extension.logger.get()))
      {
        extension.logger->Error("pool_add: the entity is no pool, or the kind has no name");
        return 0;
      }
      return PoolSystem::GetCount(*store, pool, kind) - before;
    }

    NeonEntity pool_acquire(void *context, const NeonEntity pool, const char *kind)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "pool_acquire");
      if (store == nullptr || kind == nullptr) { return 0; }
      return PoolSystem::Acquire(*store, pool, kind);
    }

    int pool_release(void *context, const NeonEntity instance)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "pool_release");
      return store != nullptr && PoolSystem::Release(*store, instance) ? 1 : 0;
    }

    std::uint64_t pool_count(void *context, const NeonEntity pool, const char *kind)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "pool_count");
      return store == nullptr || kind == nullptr ? 0 : PoolSystem::GetCount(*store, pool, kind);
    }

    std::uint64_t pool_free_count(void *context, const NeonEntity pool, const char *kind)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "pool_free_count");
      return store == nullptr || kind == nullptr ? 0 : PoolSystem::GetFreeCount(*store, pool, kind);
    }

    int set_window_mode(void *context, const std::int32_t mode)
    {
      const auto &extension = of(context);
      if (extension.services->window == nullptr)
      {
        extension.logger->Error("set_window_mode was called, and this application has no window for its extensions");
        return 0;
      }
      if (mode < 0 || mode > 2)
      {
        extension.logger->Error("set_window_mode: {} is no mode. 0 is a window, 1 without borders, 2 the whole display", mode);
        return 0;
      }
      return extension.services->window->SetWindowMode(static_cast<WindowMode>(mode)) ? 1 : 0;
    }

    std::int32_t get_window_mode(void *context)
    {
      const auto &extension = of(context);
      if (extension.services->window == nullptr) { return 0; }
      return static_cast<std::int32_t>(extension.services->window->GetWindowMode());
    }

    int set_window_size(void *context, const std::int32_t width, const std::int32_t height)
    {
      const auto &extension = of(context);
      if (extension.services->window == nullptr)
      {
        extension.logger->Error("set_window_size was called, and this application has no window for its extensions");
        return 0;
      }
      if (width <= 0 || height <= 0)
      {
        extension.logger->Error("set_window_size: {} by {} is no size, both have to be above zero", width, height);
        return 0;
      }
      return extension.services->window->SetWindowSize(width, height) ? 1 : 0;
    }

    int get_window_size(void *context, std::int32_t *width, std::int32_t *height)
    {
      const auto &extension = of(context);
      if (extension.services->window == nullptr) { return 0; }

      const WindowSize size = extension.services->window->GetWindowSize();
      if (width != nullptr) { *width = size.width; }
      if (height != nullptr) { *height = size.height; }
      return 1;
    }

    std::uint64_t list_display_sizes(
      void *context,
      void (*visit)(void *user, std::int32_t width, std::int32_t height),
      void *user)
    {
      const auto &extension = of(context);
      if (extension.services->window == nullptr) { return 0; }

      const std::vector<WindowSize> sizes = extension.services->window->GetDisplaySizes();
      if (visit != nullptr)
      {
        for (const WindowSize &size : sizes) { visit(user, size.width, size.height); }
      }
      return sizes.size();
    }

    int set_vertical_sync(void *context, const int enabled)
    {
      const auto &extension = of(context);
      if (extension.services->render == nullptr)
      {
        extension.logger->Error("set_vertical_sync was called, and this application has no renderer for its extensions");
        return 0;
      }
      return extension.services->render->SetVerticalSync(enabled != 0) ? 1 : 0;
    }

    int get_vertical_sync(void *context)
    {
      const auto &extension = of(context);
      return extension.services->render != nullptr && extension.services->render->GetVerticalSync() ? 1 : 0;
    }

    int set_frame_limit(void *context, const std::int32_t frames_per_second)
    {
      const auto &extension = of(context);
      if (extension.services->window == nullptr)
      {
        extension.logger->Error("set_frame_limit was called, and this application has no window for its extensions");
        return 0;
      }
      extension.services->window->SetFrameLimit(frames_per_second);
      return 1;
    }

    std::int32_t get_frame_limit(void *context)
    {
      const auto &extension = of(context);
      return extension.services->window == nullptr ? 0 : extension.services->window->GetFrameLimit();
    }

    NeonQuery create_query(
      void *context,
      const NeonComponent *components,
      const std::uint64_t component_count,
      const NeonQueryOrder order)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "create_query");
      if (store == nullptr) { return 0; }

      if (components == nullptr || component_count == 0 || component_count > NEON_QUERY_MAX_COMPONENTS)
      {
        const int most = NEON_QUERY_MAX_COMPONENTS;
        extension.logger->Error("create_query takes from 1 to {} components, and was given {}", most, component_count);
        return 0;
      }

      QueryInfo info;
      info.order = order == NEON_QUERY_PARENTS_FIRST ? QueryOrder::ParentsFirst : QueryOrder::Any;
      for (std::uint64_t i = 0; i < component_count; i++)
      {
        if (components[i] == No_Component)
        {
          extension.logger->Error("create_query was given no component at place {}", i);
          return 0;
        }
        info.components.push_back(components[i]);
      }

      // 0 stands for no query on the side of the extension, and is a query
      // like any other on the side of the store
      return static_cast<NeonQuery>(store->CreateQuery(info)) + 1;
    }

    void each(
      void *context,
      const NeonQuery query,
      void (*visit)(void *user, const NeonEntityBlock *block),
      void *user)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "each");
      if (store == nullptr) { return; }

      if (query == 0 || visit == nullptr)
      {
        extension.logger->Error("each was called without a query, or without a function to hand the entities to");
        return;
      }

      store->Each(static_cast<QueryId>(query - 1), [visit, user](const EntityBlock &block)
      {
        NeonEntityBlock handed{};
        handed.count = block.count;
        handed.entities = block.entities;
        handed.parent = block.parent;
        for (std::size_t i = 0; i < EntityBlock::Max_Components; i++) { handed.columns[i] = block.columns[i]; }

        visit(user, &handed);
      });
    }

    int register_system(void *context, const NeonSystemDescription *description)
    {
      auto &extension = of(context);

      if (description == nullptr || description->name == nullptr || description->name[0] == '\0')
      {
        extension.logger->Error("A system was added without a name");
        return 0;
      }

      const std::string name = description->name;
      if (extension.started)
      {
        extension.logger->Error(
          "The system '{}' was added after the world came up. Systems are added while the extension is started, "
          "or in 'register_components'",
          name);
        return 0;
      }
      if (description->update == nullptr && description->fixed_update == nullptr && description->interpolate == nullptr)
      {
        extension.logger->Error("The system '{}' has no function to call", name);
        return 0;
      }

      extension.systems.push_back(*description);
      extension.logger->Info("Added the system '{}'", name);
      return 1;
    }

    NeonField find_field(void *context, const char *component, const char *path)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "find_field");
      if (store == nullptr || component == nullptr || path == nullptr) { return 0; }

      const ComponentFormats *formats = extension.services->formats;
      const ComponentFormat *format = formats == nullptr ? nullptr : formats->Find(component);
      const ComponentId id = store->FindComponent(component);
      if (format == nullptr || format->type == nullptr || id == No_Component) { return 0; }

      const FieldInfo *field = format->type->Find(path);
      if (field == nullptr || !field->get || !field->set) { return 0; }

      extension.fields.push_back({id, std::string(component) + "." + path, format->type, field});
      return extension.fields.size();
    }

    /// The field an extension names and the component of the entity it is
    /// read from, or false after saying what is wrong. An entity without
    /// the component is not an error for reading, and `component` is then
    /// nullptr.
    bool field_of(
      LoadedExtension &extension,
      const std::string &function,
      const NeonEntity entity,
      const NeonField field,
      const ExtensionField *&found,
      void *&component)
    {
      auto *store = store_for(extension, function);
      if (store == nullptr) { return false; }

      if (field == 0 || field > extension.fields.size())
      {
        extension.logger->Error("{} was called with no field. A field is found with find_field first", function);
        return false;
      }
      if (!store->IsAlive(entity))
      {
        extension.logger->Error("{} was called with an entity that is not alive", function);
        return false;
      }

      found = &extension.fields[field - 1];
      // its fields are read and written while it is turned off as well
      component = store->GetComponentData(entity, found->component);
      return true;
    }

    int get_field(void *context, const NeonEntity entity, const NeonField field, double *numbers)
    {
      auto &extension = of(context);
      const ExtensionField *found = nullptr;
      void *component = nullptr;
      if (numbers == nullptr || !field_of(extension, "get_field", entity, field, found, component)) { return 0; }
      if (component == nullptr) { return 0; }

      return static_cast<int>(ToNumbers(found->field->get(component), numbers));
    }

    /// Sets a field after asking it whether it takes the value.
    int set_checked(
      LoadedExtension &extension,
      const std::string &function,
      const ExtensionField &found,
      void *component,
      const FieldValue &value)
    {
      if (component == nullptr)
      {
        extension.logger->Error("{} of {}: the entity has no such component", function, found.name);
        return 0;
      }
      if (const std::string problem = found.field->Check(value, found.name); !problem.empty())
      {
        extension.logger->Error("{}: {}", function, problem);
        return 0;
      }

      found.field->set(component, value);
      if (found.type != nullptr && found.type->written) { found.type->written(component); }
      return 1;
    }

    int set_field(void *context, const NeonEntity entity, const NeonField field, const double *numbers)
    {
      auto &extension = of(context);
      const ExtensionField *found = nullptr;
      void *component = nullptr;
      if (numbers == nullptr || !field_of(extension, "set_field", entity, field, found, component)) { return 0; }

      FieldValue value;
      if (!FromNumbers(found->field->kind, numbers, value))
      {
        extension.logger->Error("set_field of {}: the field holds no numbers", found->name);
        return 0;
      }
      return set_checked(extension, "set_field", *found, component, value);
    }

    std::uint64_t get_field_text(
      void *context,
      const NeonEntity entity,
      const NeonField field,
      char *buffer,
      const std::uint64_t capacity)
    {
      auto &extension = of(context);
      const ExtensionField *found = nullptr;
      void *component = nullptr;
      if (!field_of(extension, "get_field_text", entity, field, found, component) || component == nullptr) { return 0; }

      const FieldValue value = found->field->get(component);
      const auto *text = std::get_if<std::string>(&value);
      if (text == nullptr) { return 0; }

      if (buffer != nullptr && capacity > 0)
      {
        const std::size_t copied = std::min<std::size_t>(text->size(), capacity - 1);
        text->copy(buffer, copied);
        buffer[copied] = '\0';
      }
      return text->size();
    }

    int set_field_text(void *context, const NeonEntity entity, const NeonField field, const char *text)
    {
      auto &extension = of(context);
      const ExtensionField *found = nullptr;
      void *component = nullptr;
      if (text == nullptr || !field_of(extension, "set_field_text", entity, field, found, component)) { return 0; }

      return set_checked(extension, "set_field_text", *found, component, FieldValue(std::string(text)));
    }

    /// The input, or nullptr after saying that there is none.
    InputContext *input_for(LoadedExtension &extension, const std::string &function, const char *action)
    {
      if (store_for(extension, function) == nullptr || action == nullptr) { return nullptr; }

      if (extension.services->input == nullptr)
      {
        extension.logger->Error("{} was called, and this application has no input for its extensions", function);
      }
      return extension.services->input;
    }

    int is_action_down(void *context, const char *action)
    {
      auto *input = input_for(of(context), "is_action_down", action);
      return input != nullptr && input->IsActionDown(action) ? 1 : 0;
    }

    int was_action_pressed(void *context, const char *action)
    {
      auto *input = input_for(of(context), "was_action_pressed", action);
      return input != nullptr && input->WasActionPressed(action) ? 1 : 0;
    }

    std::int32_t get_input_device(void *context)
    {
      const auto &extension = of(context);
      InputContext *input = extension.services->input;
      if (input == nullptr || input->GetDevice() != InputDevice::Gamepad) { return NEON_DEVICE_KEYBOARD_AND_MOUSE; }

      switch (input->GetGamepadKind())
      {
        case GamepadKind::Xbox: return NEON_DEVICE_XBOX;
        case GamepadKind::PlayStation4: return NEON_DEVICE_PLAYSTATION_4;
        case GamepadKind::PlayStation5: return NEON_DEVICE_PLAYSTATION_5;
        case GamepadKind::Switch: return NEON_DEVICE_SWITCH;
        default: return NEON_DEVICE_GAMEPAD;
      }
    }

    float action_axis(void *context, const char *action)
    {
      auto *input = input_for(of(context), "action_axis", action);
      return input == nullptr ? 0.0f : input->ActionAxis(action);
    }

    NeonVector2 action_axis2(void *context, const char *action)
    {
      auto *input = input_for(of(context), "action_axis2", action);
      if (input == nullptr) { return {}; }

      const glm::vec2 axis = input->ActionAxis2(action);
      return {axis.x, axis.y};
    }

    NeonVector3 action_axis3(void *context, const char *action)
    {
      auto *input = input_for(of(context), "action_axis3", action);
      if (input == nullptr) { return {}; }

      const glm::vec3 axis = input->ActionAxis3(action);
      return {axis.x, axis.y, axis.z};
    }

    // Files are read at any time, since the file system is there before
    // the extensions are.

    int file_exists(void *context, const char *path)
    {
      const auto &extension = of(context);
      return path != nullptr && extension.services->file_system->Exists(path) ? 1 : 0;
    }

    int read_file(
      void *context,
      const char *path,
      void (*receive)(void *user, const std::uint8_t *bytes, std::uint64_t size),
      void *user)
    {
      const auto &extension = of(context);
      if (path == nullptr || receive == nullptr) { return 0; }

      std::vector<unsigned char> bytes;
      if (!extension.services->file_system->ReadBytes(path, bytes)) { return 0; }

      receive(user, bytes.data(), bytes.size());
      return 1;
    }

    /// Whether a virtual path has a segment `..`, which would leave the
    /// folder of its scheme.
    bool climbs_out(const std::string_view path)
    {
      std::size_t start = 0;
      while (start <= path.size())
      {
        const std::size_t end = std::min(path.find_first_of("/\\", start), path.size());
        if (path.substr(start, end - start) == "..") { return true; }
        start = end + 1;
      }
      return false;
    }

    // An extension writes under user:// alone, which is what a game may
    // write. The file system would write under output:// as well, which is
    // for what a run hands back to whoever started it, and so is not the
    // game's. What is refused is said here, in the log of the extension.
    int write_file(void *context, const char *path, const std::uint8_t *bytes, const std::uint64_t size)
    {
      const auto &extension = of(context);
      if (path == nullptr || path[0] == '\0')
      {
        extension.logger->Error("write_file was called without a path");
        return 0;
      }
      if (bytes == nullptr && size != 0)
      {
        extension.logger->Error("write_file was called without bytes for '{}'", path);
        return 0;
      }

      const std::string_view written(path);
      if (!written.starts_with(FileSystem::user_scheme))
      {
        extension.logger->Error(
          "write_file: '{}' is not under {}, which is the one place an extension writes",
          path,
          FileSystem::user_scheme);
        return 0;
      }
      if (climbs_out(written.substr(FileSystem::user_scheme.size())))
      {
        extension.logger->Error("write_file: '{}' has '..' in it, which would leave the folder of the player", path);
        return 0;
      }

      const std::vector<unsigned char> contents(bytes, bytes + size);
      if (!extension.services->file_system->WriteBytes(path, contents))
      {
        extension.logger->Error("write_file: the file system did not write '{}'", path);
        return 0;
      }
      return 1;
    }

    int list_files(void *context, const char *folder, void (*visit)(void *user, const char *name), void *user)
    {
      const auto &extension = of(context);
      if (folder == nullptr || visit == nullptr) { return 0; }

      // the file system lists a folder with everything below it, the files
      // of the folder itself first
      std::vector<std::string> paths;
      if (!extension.services->file_system->ListFiles(folder, paths)) { return 0; }

      // a folder may be written with a slash at its end, as the file system
      // takes it; a scheme alone keeps its two
      std::string prefix = folder;
      while (prefix.ends_with('/') && !prefix.ends_with("://")) { prefix.pop_back(); }
      if (!prefix.ends_with('/')) { prefix += '/'; }

      for (const auto &path : paths)
      {
        if (!path.starts_with(prefix)) { continue; }

        const std::string name = path.substr(prefix.size());
        if (name.find('/') != std::string::npos) { continue; }
        visit(user, name.c_str());
      }
      return 1;
    }

    // The window is told to close, which is what the quit of the pause
    // menu does: the application leaves its loop once the frame is done.
    void request_quit(void *context)
    {
      const auto &extension = of(context);
      if (extension.services->window == nullptr)
      {
        extension.logger->Error("request_quit was called, and this application has no window for its extensions");
        return;
      }

      extension.logger->Info("The extension asked the application to close");
      extension.services->window->SignalToClose();
    }

    /// The world, or nullptr after saying that there is none.
    WorldSystem *world_for(LoadedExtension &extension, const std::string &function, const char *path)
    {
      if (store_for(extension, function) == nullptr || path == nullptr) { return nullptr; }

      if (extension.services->world == nullptr)
      {
        extension.logger->Error("{} was called, and this application has no world for its extensions", function);
      }
      return extension.services->world;
    }

    NeonEntity spawn(void *context, const char *prefab_path, const NeonEntity parent)
    {
      auto *world = world_for(of(context), "spawn", prefab_path);
      return world == nullptr ? No_Entity : world->Spawn(prefab_path, parent, DataValue{});
    }

    void load_scene(void *context, const char *scene_path)
    {
      if (auto *world = world_for(of(context), "load_scene", scene_path)) { world->LoadScene(scene_path); }
    }

    int listen_to_physics(
      void *context,
      void (*listen)(void *user, const NeonPhysicsEvent *event),
      void *user)
    {
      auto &extension = of(context);

      if (listen == nullptr)
      {
        extension.logger->Error("listen_to_physics was called without a function to tell");
        return 0;
      }
      if (extension.started)
      {
        extension.logger->Error(
          "listen_to_physics was called after the world came up. It is called where systems are added");
        return 0;
      }

      extension.physics_listeners.push_back({listen, user});
      return 1;
    }

    int cast_ray(void *context, const NeonRay *ray, NeonRayHit *hit)
    {
      auto &extension = of(context);
      if (store_for(extension, "cast_ray") == nullptr || ray == nullptr || hit == nullptr) { return 0; }

      PhysicsContext *physics = extension.services->physics;
      if (physics == nullptr) { return 0; }

      const glm::vec3 direction(ray->direction.x, ray->direction.y, ray->direction.z);
      if (direction == glm::vec3(0.0f))
      {
        extension.logger->Error("cast_ray was called with a direction of no length");
        return 0;
      }

      const Ray cast{
        .origin = {ray->origin.x, ray->origin.y, ray->origin.z},
        .direction = direction,
        .distance = ray->distance
      };
      const QueryFilter filter{.mask = ray->layers, .triggers = ray->triggers != 0, .ignore = ray->ignore};

      RayHit found;
      if (!physics->CastRay(cast, filter, found)) { return 0; }

      hit->entity = found.entity;
      hit->trigger = found.trigger ? 1 : 0;
      hit->point = {found.point.x, found.point.y, found.point.z};
      hit->normal = {found.normal.x, found.normal.y, found.normal.z};
      hit->distance = found.distance;
      return 1;
    }

    /// The kind of the C file for a kind of the engine, the other way than
    /// kind_of. Returns false for one the C file has no type for.
    bool described_kind_of(const FieldKind kind, NeonFieldKind &described)
    {
      for (int candidate = NEON_FIELD_BOOLEAN; candidate <= NEON_FIELD_INTEGER_VECTOR3; candidate++)
      {
        FieldKind of_candidate;
        if (kind_of(static_cast<NeonFieldKind>(candidate), of_candidate) && of_candidate == kind)
        {
          described = static_cast<NeonFieldKind>(candidate);
          return true;
        }
      }
      return false;
    }

    int get_field_layout(void *context, const NeonField field, NeonFieldLayout *layout)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "get_field_layout");
      if (store == nullptr || layout == nullptr) { return 0; }

      if (field == 0 || field > extension.fields.size())
      {
        extension.logger->Error("get_field_layout was called with no field. A field is found with find_field first");
        return 0;
      }

      const ExtensionField &found = extension.fields[field - 1];
      const std::size_t stride = store->GetComponentSize(found.component);
      NeonFieldKind kind;
      if (!found.field->reach || stride == 0 || !described_kind_of(found.field->kind, kind)) { return 0; }

      // Where the member lies is asked of the description, with memory of
      // the size of a component that is looked at and never read: reaching
      // a member is arithmetic on the address alone.
      std::vector<std::max_align_t> memory((stride + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t));
      const auto *start = reinterpret_cast<const char *>(memory.data());
      const auto *member = static_cast<const char *>(found.field->reach(memory.data()));
      if (member < start || member >= start + stride) { return 0; }

      layout->kind = kind;
      layout->offset = static_cast<std::uint64_t>(member - start);
      layout->stride = stride;
      return 1;
    }

    /// The user interface, or nullptr after saying that there is none.
    UiContext *ui_for(LoadedExtension &extension, const std::string &function, const char *name)
    {
      if (store_for(extension, function) == nullptr || name == nullptr) { return nullptr; }

      if (extension.services->ui == nullptr)
      {
        extension.logger->Error("{} was called, and this application has no user interface for its extensions", function);
      }
      return extension.services->ui;
    }

    void set_ui_number(void *context, const char *name, const double number)
    {
      if (auto *ui = ui_for(of(context), "set_ui_number", name)) { ui->SetNumber(name, number); }
    }

    void set_ui_text(void *context, const char *name, const char *text)
    {
      if (auto *ui = ui_for(of(context), "set_ui_text", name)) { ui->SetText(name, text == nullptr ? "" : text); }
    }

    // The elements of the user interface are named by the numbers of the
    // handles of the engine, which are given once and never again.
    static_assert(sizeof(NeonUiElement) == sizeof(UiHandle::id));

    int ui_show(void *context, const char *path)
    {
      auto &extension = of(context);
      auto *ui = ui_for(extension, "ui_show", path);
      if (ui == nullptr) { return 0; }

      if (path[0] == '\0')
      {
        extension.logger->Error("ui_show was called without a path");
        return 0;
      }

      // what is shown already stays; what cancel closed meanwhile is shown
      // again
      const auto shown = extension.ui_documents.find(path);
      if (shown != extension.ui_documents.end() && ui->IsShown(shown->second)) { return 1; }

      const int document = ui->Load(path);
      if (document < 0)
      {
        extension.ui_documents.erase(path);
        extension.logger->Error("ui_show: the user interface did not take '{}', and said why", path);
        return 0;
      }

      extension.ui_documents[path] = document;
      return 1;
    }

    int ui_close(void *context, const char *path)
    {
      auto &extension = of(context);
      auto *ui = ui_for(extension, "ui_close", path);
      if (ui == nullptr) { return 0; }

      const auto shown = extension.ui_documents.find(path);
      if (shown == extension.ui_documents.end())
      {
        extension.logger->Error("ui_close: the extension shows no file '{}'. A file is shown with ui_show first", path);
        return 0;
      }

      ui->Unload(shown->second);
      extension.ui_documents.erase(shown);
      return 1;
    }

    NeonUiElement ui_find(void *context, const char *name)
    {
      auto *ui = ui_for(of(context), "ui_find", name);
      return ui == nullptr ? 0 : ui->FindByName(name).id;
    }

    /// The user interface when the element is still there, or nullptr after
    /// saying that it is gone.
    UiContext *ui_with(LoadedExtension &extension, const std::string &function, const NeonUiElement element)
    {
      auto *ui = ui_for(extension, function, "");
      if (ui == nullptr) { return nullptr; }

      if (!ui->IsAlive(UiHandle{element}))
      {
        extension.logger->Error(
          "{} was called with an element that is not there: it was never found or created, or it is gone", function);
        return nullptr;
      }
      return ui;
    }

    NeonUiElement ui_create(void *context, const char *yaml, const NeonUiElement parent)
    {
      auto &extension = of(context);
      auto *ui = ui_for(extension, "ui_create", yaml);
      if (ui == nullptr) { return 0; }

      if (std::string_view(yaml).find_first_not_of(" \t\r\n") == std::string_view::npos)
      {
        extension.logger->Error("ui_create was called without text to make an element from");
        return 0;
      }

      // no parent is the element at the top of the topmost file
      UiHandle into{parent};
      if (parent == 0)
      {
        into = ui->GetRoot();
        if (!into.IsSet())
        {
          extension.logger->Error(
            "ui_create: no file of the user interface is shown, so there is nothing to put an element into. One is "
            "shown with ui_show first");
          return 0;
        }
      } else if (!ui->IsAlive(into))
      {
        extension.logger->Error("ui_create was called with a parent that is not there");
        return 0;
      }

      const UiHandle made = ui->Create(yaml, into);
      if (!made.IsSet())
      {
        extension.logger->Error("ui_create: the user interface made no element from the text, and said why");
      }
      return made.id;
    }

    int ui_remove(void *context, const NeonUiElement element)
    {
      auto &extension = of(context);
      auto *ui = ui_with(extension, "ui_remove", element);
      if (ui == nullptr) { return 0; }

      if (!ui->Remove(UiHandle{element}))
      {
        extension.logger->Error("ui_remove: the element was not removed. The one at the top of a file is closed with it");
        return 0;
      }
      return 1;
    }

    // A field is set from text, whatever it holds, as a file writes it. What
    // the kind of the element says the field holds is what the text is read
    // as, so that one function does for texts, numbers, and flags.
    int ui_set_field(void *context, const NeonUiElement element, const char *field, const char *text)
    {
      auto &extension = of(context);
      if (field == nullptr || text == nullptr) { return 0; }

      auto *ui = ui_with(extension, "ui_set_field", element);
      if (ui == nullptr) { return 0; }

      const UiHandle handle{element};
      const std::string type = ui->GetElementType(handle);

      TypeInfo described;
      const FieldInfo *known = ui->DescribeElement(type, described) ? described.Find(field) : nullptr;
      if (known == nullptr || known->kind == FieldKind::Group)
      {
        extension.logger->Error("ui_set_field: '{}' is not a field of a {}", field, type);
        return 0;
      }

      FieldValue value;
      const std::string what = "'" + std::string(field) + "' of a " + type;
      if (std::string error; !ParseField(text, *known, what, value, error))
      {
        extension.logger->Error("ui_set_field: {}", error);
        return 0;
      }

      if (!ui->SetField(handle, field, value))
      {
        extension.logger->Error("ui_set_field: {} did not take '{}', and the user interface said why", what, text);
        return 0;
      }
      return 1;
    }

    int ui_set_style(void *context, const NeonUiElement element, const char *property, const char *text)
    {
      auto &extension = of(context);
      if (property == nullptr) { return 0; }

      auto *ui = ui_with(extension, "ui_set_style", element);
      if (ui == nullptr) { return 0; }

      const std::string value = text == nullptr ? "" : text;
      if (!ui->Set(UiHandle{element}, property, value))
      {
        extension.logger->Error(
          "ui_set_style: '{}' was not set to '{}': it is no property that is known, or cannot hold the value",
          property,
          value);
        return 0;
      }
      return 1;
    }

    int ui_set_visible(void *context, const NeonUiElement element, const int visible)
    {
      auto *ui = ui_with(of(context), "ui_set_visible", element);
      return ui != nullptr && ui->SetVisible(UiHandle{element}, visible != 0) ? 1 : 0;
    }

    NeonUiListener ui_listen(
      void *context,
      const NeonUiElement element,
      const char *event,
      void (*listen)(void *user, const NeonUiEvent *event),
      void *user)
    {
      auto &extension = of(context);

      if (listen == nullptr || event == nullptr || event[0] == '\0')
      {
        extension.logger->Error("ui_listen was called without an event, or without a function to tell");
        return 0;
      }

      auto *ui = ui_with(extension, "ui_listen", element);
      if (ui == nullptr) { return 0; }

      const int listening = ui->On(UiHandle{element}, event, [listen, user](const UiElementEvent &happened)
      {
        NeonUiEvent told{};
        told.target = happened.target.id;
        told.name = happened.name.c_str();
        told.x = happened.x;
        told.y = happened.y;
        listen(user, &told);
      });
      if (listening <= 0)
      {
        extension.logger->Error("ui_listen: the user interface does not tell of '{}' on the element", event);
        return 0;
      }

      extension.ui_listeners.push_back(listening);
      return static_cast<NeonUiListener>(listening);
    }

    // Only what the extension itself listens with is taken away, so that
    // a number that was made up cannot take away what the game, a script,
    // or another extension listens with.
    void ui_unlisten(void *context, const NeonUiListener listener)
    {
      auto &extension = of(context);
      const auto kept = std::ranges::find_if(
        extension.ui_listeners, [listener](const int each) { return static_cast<NeonUiListener>(each) == listener; });
      if (kept == extension.ui_listeners.end())
      {
        extension.logger->Error("ui_unlisten was called with what the extension does not listen with");
        return;
      }

      // the user interface may be gone at the end, and what listens with it
      if (extension.services->ui != nullptr) { extension.services->ui->Off(*kept); }
      extension.ui_listeners.erase(kept);
    }

    // What the camera of the window draws to is what the renderer draws to,
    // which knows its size from the window and when it changes.
    int get_view_size(void *context, std::int32_t *width, std::int32_t *height)
    {
      auto &extension = of(context);
      if (extension.services->render == nullptr)
      {
        extension.logger->Error("get_view_size was called, and this application has no renderer for its extensions");
        return 0;
      }

      const RenderResolution &resolution = extension.services->render->GetRenderResolution();
      if (width != nullptr) { *width = resolution.width; }
      if (height != nullptr) { *height = resolution.height; }
      return 1;
    }

    DataValue list_of(const NeonVector3 &vector)
    {
      auto list = DataValue::List();
      list.Add(DataValue::Number(vector.x));
      list.Add(DataValue::Number(vector.y));
      list.Add(DataValue::Number(vector.z));
      return list;
    }

    NeonEntity spawn_at(
      void *context,
      const char *prefab_path,
      const NeonEntity parent,
      const NeonVector3 *position,
      const NeonVector3 *rotation)
    {
      auto *world = world_for(of(context), "spawn_at", prefab_path);
      if (world == nullptr) { return No_Entity; }

      // written on top of the prefab as a recipe would, so that the entity
      // is where it belongs from the moment it exists
      auto transform = DataValue::Map();
      if (position != nullptr) { transform.Set("position", list_of(*position)); }
      if (rotation != nullptr) { transform.Set("rotation", list_of(*rotation)); }

      auto overrides = DataValue::Map();
      overrides.Set("Transform", transform);
      return world->Spawn(prefab_path, parent, overrides);
    }

    int add_component(void *context, const NeonEntity entity, const char *component)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "add_component");
      if (store == nullptr || component == nullptr) { return 0; }

      if (!store->IsAlive(entity))
      {
        extension.logger->Error("add_component was called with an entity that is not alive");
        return 0;
      }

      const ComponentFormats *formats = extension.services->formats;
      const ComponentFormat *format = formats == nullptr ? nullptr : formats->Find(component);
      const ComponentId id = store->FindComponent(component);
      if (format == nullptr || id == No_Component)
      {
        extension.logger->Error("add_component: there is no component '{}' that a recipe could write", component);
        return 0;
      }

      if (store->HasComponent(entity, id)) { return 1; }

      // Read as a recipe that names the component and nothing of it, which
      // gives every field what it starts with. What a recipe would be told
      // is missing, the shader of a Renderable, is not a problem here: the
      // extension sets its fields next.
      std::vector<std::string> errors;
      const DataValue nothing = DataValue::Map();
      format->read(DataReader(nothing, "add_component", component, errors), *store, entity);
      if (store->HasComponent(entity, id)) { return 1; }

      for (const auto &error : errors) { extension.logger->Error("add_component: {}", error); }
      extension.logger->Error("add_component: the component '{}' could not be given to the entity", component);
      return 0;
    }

    int set_field_texts(
      void *context,
      const NeonEntity entity,
      const NeonField field,
      const char *const *texts,
      const std::uint64_t count)
    {
      auto &extension = of(context);
      const ExtensionField *found = nullptr;
      void *component = nullptr;
      if ((texts == nullptr && count > 0) || !field_of(extension, "set_field_texts", entity, field, found, component))
      {
        return 0;
      }

      std::vector<std::string> list;
      for (std::uint64_t i = 0; i < count; i++) { list.emplace_back(texts[i] == nullptr ? "" : texts[i]); }

      return set_checked(extension, "set_field_texts", *found, component, FieldValue(list));
    }

    int set_image(
      void *context,
      const char *name,
      const std::uint32_t width,
      const std::uint32_t height,
      const std::uint8_t *pixels)
    {
      auto &extension = of(context);
      if (store_for(extension, "set_image") == nullptr) { return 0; }

      if (name == nullptr || name[0] == '\0' || pixels == nullptr || width == 0 || height == 0)
      {
        extension.logger->Error("set_image was called without a name, or without pixels");
        return 0;
      }
      if (extension.services->render == nullptr)
      {
        extension.logger->Error("set_image was called, and this application has no renderer for its extensions");
        return 0;
      }

      ImagePixels image;
      image.width = static_cast<int>(width);
      image.height = static_cast<int>(height);
      image.pixels.assign(pixels, pixels + static_cast<std::size_t>(width) * height * 4);

      const std::string full_name = extension.name + "/" + name;
      if (!extension.services->render->SetImage(full_name, image))
      {
        extension.logger->Error("set_image: the renderer did not take the picture '{}'", full_name);
        return 0;
      }
      return 1;
    }

    int set_sound(void *context, const char *name, const std::uint8_t *bytes, const std::uint64_t size)
    {
      auto &extension = of(context);
      if (store_for(extension, "set_sound") == nullptr) { return 0; }

      if (name == nullptr || name[0] == '\0' || bytes == nullptr || size == 0)
      {
        extension.logger->Error("set_sound was called without a name, or without bytes");
        return 0;
      }
      if (extension.services->audio == nullptr)
      {
        extension.logger->Error("set_sound was called, and this application has no audio for its extensions");
        return 0;
      }

      const std::string full_name = extension.name + "/" + name;
      if (!extension.services->audio->SetSound(full_name, std::vector<std::uint8_t>(bytes, bytes + size)))
      {
        extension.logger->Error("set_sound: the audio did not take the sound '{}'", full_name);
        return 0;
      }
      return 1;
    }

    int set_group_volume(void *context, const char *group, const float volume)
    {
      auto &extension = of(context);
      if (group == nullptr || group[0] == '\0')
      {
        extension.logger->Error("set_group_volume was called without the name of a group");
        return 0;
      }
      if (extension.services->audio == nullptr)
      {
        extension.logger->Error("set_group_volume was called, and this application has no audio for its extensions");
        return 0;
      }

      extension.services->audio->SetGroupVolume(group, volume);
      return 1;
    }

    float get_group_volume(void *context, const char *group)
    {
      const auto &extension = of(context);
      if (group == nullptr || group[0] == '\0' || extension.services->audio == nullptr) { return 0.0f; }

      return extension.services->audio->GetGroupVolume(group);
    }

    int free_unused(void *context)
    {
      const auto &extension = of(context);
      if (extension.services->render == nullptr)
      {
        extension.logger->Error("free_unused was called, and this application has no renderer for its extensions");
        return 0;
      }

      extension.services->render->FreeUnused();
      return 1;
    }

    int set_shader_numbers(
      void *context, const std::int32_t place, const float x, const float y, const float z, const float w)
    {
      auto &extension = of(context);
      if (extension.services->render == nullptr)
      {
        extension.logger->Error(
          "set_shader_numbers was called, and this application has no renderer for its extensions");
        return 0;
      }
      if (!extension.services->render->SetShaderNumbers(place, {x, y, z, w}))
      {
        const int last = RenderContext::kShader_Number_Places - 1;
        extension.logger->Error(
          "set_shader_numbers: there is no place {} for the numbers of a game, the places are 0 to {}",
          place, last);
        return 0;
      }
      return 1;
    }

    // A corner of the extension is the start of a corner of the engine,
    // which has grown since: what follows keeps what it starts with.
    static_assert(sizeof(NeonVertex) <= sizeof(Vertex));
    static_assert(offsetof(NeonVertex, normal) == offsetof(Vertex, normal));
    static_assert(offsetof(NeonVertex, texture) == offsetof(Vertex, tex_coords));
    static_assert(offsetof(NeonVertex, color) == offsetof(Vertex, color));

    int set_mesh(
      void *context,
      const NeonEntity entity,
      const NeonVertex *vertices,
      const std::uint64_t vertex_count,
      const std::uint32_t *indices,
      const std::uint64_t index_count)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "set_mesh");
      if (store == nullptr) { return 0; }

      if (!store->IsAlive(entity))
      {
        extension.logger->Error("set_mesh was called with an entity that is not alive");
        return 0;
      }
      if (vertices == nullptr || indices == nullptr || vertex_count == 0 || index_count == 0 || index_count % 3 != 0)
      {
        extension.logger->Error(
          "set_mesh takes corners, and three indices for every triangle; it was given {} corners and {} indices",
          vertex_count,
          index_count);
        return 0;
      }

      const ComponentId id = store->FindComponent("Renderable");
      auto *renderable = id == No_Component ? nullptr : static_cast<Renderable *>(store->GetComponentData(entity, id));
      if (renderable == nullptr)
      {
        extension.logger->Error("set_mesh: the entity has no Renderable to draw the mesh with");
        return 0;
      }

      for (std::uint64_t i = 0; i < index_count; i++)
      {
        if (indices[i] >= vertex_count)
        {
          const std::uint32_t index = indices[i];
          extension.logger->Error(
            "set_mesh: index {} names corner {}, and there are {} corners",
            i,
            index,
            vertex_count);
          return 0;
        }
      }

      auto mesh = std::make_shared<MeshData>();
      mesh->vertices.resize(vertex_count);
      for (std::uint64_t i = 0; i < vertex_count; i++)
      {
        std::memcpy(&mesh->vertices[i], &vertices[i], sizeof(NeonVertex));
      }
      mesh->indices.assign(indices, indices + index_count);

      // counted up, so that an entity that is drawn already is handed the
      // new mesh, as a rope that moves is
      renderable->render_info.mesh = std::move(mesh);
      renderable->render_info.mesh_version++;
      return 1;
    }

    int set_mesh_lightmap(
      void *context,
      const NeonEntity entity,
      const NeonVector2 *coordinates,
      const std::uint64_t count)
    {
      auto &extension = of(context);
      auto *store = store_for(extension, "set_mesh_lightmap");
      if (store == nullptr) { return 0; }

      if (!store->IsAlive(entity))
      {
        extension.logger->Error("set_mesh_lightmap was called with an entity that is not alive");
        return 0;
      }

      const ComponentId id = store->FindComponent("Renderable");
      auto *renderable = id == No_Component ? nullptr : static_cast<Renderable *>(store->GetComponentData(entity, id));
      if (renderable == nullptr || renderable->render_info.mesh == nullptr)
      {
        extension.logger->Error("set_mesh_lightmap: the entity has no mesh; one is set with set_mesh first");
        return 0;
      }

      const std::size_t corners = renderable->render_info.mesh->vertices.size();
      if (coordinates == nullptr || count != corners)
      {
        extension.logger->Error(
          "set_mesh_lightmap takes one pair of coordinates for every corner; the mesh has {} corners and {} were given",
          corners,
          count);
        return 0;
      }

      // the mesh may be held by others, so it is another mesh from here on
      auto mesh = std::make_shared<MeshData>(*renderable->render_info.mesh);
      for (std::size_t i = 0; i < corners; i++)
      {
        mesh->vertices[i].lightmap_coords = {coordinates[i].x, coordinates[i].y};
      }

      renderable->render_info.mesh = std::move(mesh);
      renderable->render_info.mesh_version++;
      return 1;
    }
  }

  // the two sides count the components of a query alike, and name entities
  // and components by the same numbers
  static_assert(NEON_QUERY_MAX_COMPONENTS == EntityBlock::Max_Components);
  static_assert(sizeof(NeonEntity) == sizeof(Entity) && sizeof(NeonComponent) == sizeof(ComponentId));

  void FillExtensionHostTable(LoadedExtension &extension)
  {
    NeonExtensionHost &host = extension.host;

    host.abi_version = NEON_EXTENSION_ABI_VERSION;
    host.context = &extension;

    host.log = &log;

    host.register_component = &register_component;
    host.find_component = &find_component;
    host.create_entity = &create_entity;
    host.destroy_entity = &destroy_entity;
    host.is_alive = &is_alive;
    host.find_entity = &find_entity;
    host.set_parent = &set_parent;
    host.get_parent = &get_parent;
    host.set_component = &set_component;
    host.get_component = &get_component;
    host.has_component = &has_component;
    host.remove_component = &remove_component;
    host.create_query = &create_query;
    host.each = &each;

    host.register_system = &register_system;

    host.find_field = &find_field;
    host.get_field = &get_field;
    host.set_field = &set_field;
    host.get_field_text = &get_field_text;
    host.set_field_text = &set_field_text;
    host.is_action_down = &is_action_down;
    host.was_action_pressed = &was_action_pressed;
    host.action_axis = &action_axis;
    host.action_axis2 = &action_axis2;
    host.action_axis3 = &action_axis3;
    host.file_exists = &file_exists;
    host.read_file = &read_file;
    host.spawn = &spawn;
    host.load_scene = &load_scene;

    host.listen_to_physics = &listen_to_physics;
    host.cast_ray = &cast_ray;

    host.get_field_layout = &get_field_layout;
    host.set_ui_number = &set_ui_number;
    host.set_ui_text = &set_ui_text;
    host.spawn_at = &spawn_at;

    host.add_component = &add_component;
    host.set_field_texts = &set_field_texts;
    host.set_image = &set_image;
    host.set_mesh = &set_mesh;

    host.set_mesh_lightmap = &set_mesh_lightmap;

    host.set_sound = &set_sound;

    host.write_file = &write_file;
    host.list_files = &list_files;
    host.request_quit = &request_quit;

    host.set_shader_numbers = &set_shader_numbers;

    host.ui_show = &ui_show;
    host.ui_close = &ui_close;
    host.ui_find = &ui_find;
    host.ui_create = &ui_create;
    host.ui_remove = &ui_remove;
    host.ui_set_field = &ui_set_field;
    host.ui_set_style = &ui_set_style;
    host.ui_set_visible = &ui_set_visible;
    host.ui_listen = &ui_listen;
    host.ui_unlisten = &ui_unlisten;
    host.get_view_size = &get_view_size;

    host.set_group_volume = &set_group_volume;
    host.get_group_volume = &get_group_volume;

    host.free_unused = &free_unused;

    host.set_component_enabled = &set_component_enabled;
    host.is_component_enabled = &is_component_enabled;

    host.pool_add = &pool_add;
    host.pool_acquire = &pool_acquire;
    host.pool_release = &pool_release;
    host.pool_count = &pool_count;
    host.pool_free_count = &pool_free_count;

    host.set_window_mode = &set_window_mode;
    host.get_window_mode = &get_window_mode;
    host.set_window_size = &set_window_size;
    host.get_window_size = &get_window_size;
    host.list_display_sizes = &list_display_sizes;
    host.set_vertical_sync = &set_vertical_sync;
    host.get_vertical_sync = &get_vertical_sync;
    host.set_frame_limit = &set_frame_limit;
    host.get_frame_limit = &get_frame_limit;
    host.get_input_device = &get_input_device;

    // the extension keeps its name for as long as it is loaded
    host.name = extension.name.c_str();
  }
} // neon
