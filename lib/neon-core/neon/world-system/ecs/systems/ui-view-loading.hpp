#ifndef UI_VIEW_LOADING_HPP
#define UI_VIEW_LOADING_HPP

#include <memory>

#include <neon/logging/logger.hpp>
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
  ///     world.AddSystem(std::make_unique<neon::UiViewLoading>(&ui_system, logger));
  class UiViewLoading final : public EntitySystem
  {
    UiContext *_ui_context;
    std::shared_ptr<Logger> _logger;
    QueryId _query = 0;

  public:
    /// What the component is called in a scene file.
    static constexpr const char *kComponent_Name = "Ui";

    UiViewLoading(UiContext *ui_context, const std::shared_ptr<Logger> &logger);

    /// Registers the component, which the engine does not know by itself.
    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    /// A file that cannot be used is said once in the log, and the entity
    /// shows nothing; the game goes on.
    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //UI_VIEW_LOADING_HPP
