#ifndef SCRIPT_UI_CALL_HPP
#define SCRIPT_UI_CALL_HPP

#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity.hpp>

namespace neon
{
  /// A function of the scripts that the user interface asks for: what an
  /// element named with `on_click`, and the entity that shows the user
  /// interface the element is in, on whose systems the function is looked
  /// for.
  struct ScriptUiCall
  {
    /// The entity whose `Ui` or `UiSurface` shows the file. No_Entity for
    /// a file no entity shows, such as a menu the runtime loaded.
    Entity entity = No_Entity;

    /// The entity the click comes from: who pointed at the surface in the
    /// world the file is shown on, see UiSurfacePointing. No_Entity for a
    /// click on the window, which no entity made.
    Entity instigator = No_Entity;

    /// What happened, with the function and what it is handed in `call`.
    UiEvent event;
  };
} // neon

#endif //SCRIPT_UI_CALL_HPP
