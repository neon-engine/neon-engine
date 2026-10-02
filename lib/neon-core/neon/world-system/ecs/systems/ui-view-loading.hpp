#ifndef UI_VIEW_LOADING_HPP
#define UI_VIEW_LOADING_HPP

#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Shows the user interface of every entity that carries a `UiView`, and
  /// stops showing it when the entity or the component goes away.
  ///
  /// An application adds it to the world, together with the format of the
  /// component for its scene files:
  ///
  ///     scene.GetComponentFormats().Add(neon::UiViewLoading::Format());
  ///     world.AddSystem(std::make_unique<neon::UiViewLoading>(&ui_system));
  class UiViewLoading final : public EntitySystem
  {
    UiContext *_ui_context;
    QueryId _query = 0;

  public:
    /// What the component is called in a scene file.
    static constexpr const char *kComponent_Name = "Ui";

    explicit UiViewLoading(UiContext *ui_context);

    /// Registers the component, which the engine does not know by itself.
    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    /// Throws when a file cannot be used, as a scene does that cannot be
    /// read.
    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //UI_VIEW_LOADING_HPP
