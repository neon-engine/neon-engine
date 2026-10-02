#include "ui-view-loading.hpp"


#include <neon/world-system/ecs/components/ui-view.hpp>

namespace neon
{
  UiViewLoading::UiViewLoading(UiContext *ui_context, const std::shared_ptr<Logger> &logger)
  {
    _ui_context = ui_context;
    _logger = logger;
  }

  void UiViewLoading::Register(EntityStore &store)
  {
    store.Register<UiView>(kComponent_Name, [this](Entity, UiView &view)
    {
      if (view.document < 0) { return; }

      _ui_context->Unload(view.document);
      view.document = -1;
    });
  }

  void UiViewLoading::Initialize(EntityStore &store)
  {
    _query = store.Query<UiView>();
  }

  void UiViewLoading::Update(EntityStore &store, double delta_time)
  {
    store.Each(_query, [this](const EntityBlock &block)
    {
      auto *views = block.Column<UiView>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        UiView &view = views[i];
        if (view.is_tried) { continue; }

        view.is_tried = true;
        view.document = _ui_context->Load(view.file);

        // said once, since is_tried keeps it from being read again; the
        // entity shows nothing and the game goes on
        if (view.document < 0)
        {
          _logger->Error("The user interface {} cannot be used, the entity shows nothing", view.file);
        }
      }
    });
  }
} // neon
