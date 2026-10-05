#include "ui-surface-loading.hpp"

#include <format>

#include <neon/world-system/ecs/components/ui-surface-view.hpp>

namespace neon
{
  UiSurfaceLoading::UiSurfaceLoading(UiContext *ui_context, const std::shared_ptr<Logger> &logger)
  {
    _ui_context = ui_context;
    _logger = logger;
  }

  void UiSurfaceLoading::Register(EntityStore &store)
  {
    const auto let_go = [this](Entity, UiSurfaceView &view)
    {
      // what is shown on the surface goes with it
      if (view.surface >= 0) { _ui_context->DestroySurface(view.surface); }

      view.surface = -1;
      view.document = -1;
    };

    // a surface that is turned off shows nothing, and is kept with what
    // was read onto it: nothing is made again when it is turned on
    store.Register<UiSurfaceView>(
      kComponent_Name,
      let_go,
      [this](Entity, UiSurfaceView &view, const bool enabled)
      {
        if (view.document < 0) { return; }
        _ui_context->SetVisible(_ui_context->GetRoot(view.document), enabled);
      });
  }

  void UiSurfaceLoading::Initialize(EntityStore &store)
  {
    _query = store.Query<UiSurfaceView>();
  }

  void UiSurfaceLoading::Update(EntityStore &store, double delta_time)
  {
    store.Each(_query, [this](const EntityBlock &block)
    {
      auto *views = block.Column<UiSurfaceView>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        UiSurfaceView &view = views[i];
        if (view.is_tried) { continue; }

        view.is_tried = true;
        view.surface = _ui_context->CreateSurface(view.name, view.width, view.height, view.scale);

        // said once, since is_tried keeps it from being tried again; the
        // entity shows nothing and the game goes on
        if (view.surface < 0)
        {
          _logger->Error("The surface '{}' of a user interface cannot be made, the entity shows nothing", view.name);
          continue;
        }

        view.document = _ui_context->LoadOnto(view.surface, view.ui);

        if (view.document < 0)
        {
          _logger->Error("The user interface {} cannot be used, the surface '{}' shows nothing", view.ui, view.name);
        }
      }
    });
  }

  ComponentFormat UiSurfaceFormat()
  {

    return ComponentFormat::Of<UiSurfaceView>(
      UiSurfaceLoading::kComponent_Name,
      [](const DataReader &reader, UiSurfaceView &view)
      {
        if (!reader.Read("ui", view.ui) && !reader.Has("ui"))
        {
          reader.Report(std::format(
            "{} has no 'ui', where the virtual path of a user interface was expected", reader.GetWhere()));
        }

        if ((!reader.Read("name", view.name) && !reader.Has("name")) || (reader.Has("name") && view.name.empty()))
        {
          reader.Report(std::format(
            "{} has no 'name', where what the surface is called was expected. A model shows the surface as "
            "the texture surface:// and the name",
            reader.GetWhere()));
        }

        if (const auto *size = reader.ReadValue("size"); size != nullptr)
        {
          float width = 0.0f;
          float height = 0.0f;

          if (size->IsList() && size->GetItems().size() == 2 &&
              size->GetItems()[0].GetNumber(width) && size->GetItems()[1].GetNumber(height) &&
              width >= 1.0f && height >= 1.0f)
          {
            view.width = static_cast<int>(width);
            view.height = static_cast<int>(height);
          } else
          {
            reader.Report(*size, std::format(
                            "'size' of {} is {}, where a list of 2 numbers above 0 was expected, such as "
                            "[1024, 768]",
                            reader.GetWhere(),
                            size->IsList() ? "another list" : DataValue::Describe(size->GetKind())));
          }
        }

        if (reader.Read("reach", view.reach) && view.reach < 0.0f)
        {
          reader.Report(*reader.ReadValue("reach"), std::format(
                          "'reach' of {} is {}, where a number of 0 or above was expected",
                          reader.GetWhere(), view.reach));
          view.reach = UiSurfaceView{}.reach;
        }

        if (reader.Read("scale", view.scale) && view.scale <= 0.0f)
        {
          reader.Report(*reader.ReadValue("scale"), std::format(
                          "'scale' of {} is {}, where a number above 0 was expected",
                          reader.GetWhere(), view.scale));
          view.scale = 1.0f;
        }
      },
      [](const UiSurfaceView &view, DataValue &map)
      {
        const UiSurfaceView standard{};

        map.Set("ui", DataValue::Text(view.ui));
        map.Set("name", DataValue::Text(view.name));

        if (view.width != standard.width || view.height != standard.height)
        {
          auto size = DataValue::List();
          size.Add(DataValue::Number(view.width));
          size.Add(DataValue::Number(view.height));
          map.Set("size", size);
        }

        if (view.scale != standard.scale) { map.Set("scale", DataValue::Number(view.scale)); }
        if (view.reach != standard.reach) { map.Set("reach", DataValue::Number(view.reach)); }
      });
  }
} // neon
