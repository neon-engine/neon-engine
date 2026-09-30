#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP

#include <neon/render/render-info.hpp>

namespace neon
{
  /// Makes an entity visible. It is drawn where its Transform places it.
  struct Renderable
  {
    RenderInfo render_info;

    /// What the renderer knows the entity as. Filled in by the engine the
    /// first time the entity is drawn. -1 until then.
    int render_object_id = -1;
  };
} // neon

#endif //RENDERABLE_HPP
