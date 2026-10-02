#include "ui-view-loading.hpp"

#include <stdexcept>

#include <neon/world-system/ecs/components/ui-view.hpp>

namespace neon
{
  UiViewLoading::UiViewLoading(UiContext *ui_context)
  {
    _ui_context = ui_context;
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

        if (view.document < 0)
        {
          throw std::runtime_error("The user interface " + view.file + " cannot be used");
        }
      }
    });
  }
} // neon
