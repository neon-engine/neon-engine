#ifndef PREFAB_HPP
#define PREFAB_HPP

#include <string>

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Where an entity came from: the prefab recipe that was placed to make
  /// it. The loader sets it on the entity a prefab was placed as, so that
  /// the editor knows which file to write a change back to. It is not
  /// written under `components` in a file; `prefab:` next to the name of the
  /// entity is what places a prefab.
  struct Prefab
  {
    /// Virtual path of the prefab recipe, such as
    /// `assets://prefabs/wall.prefab.yml`.
    std::string path;
  };

  inline void Describe(TypeBuilder<Prefab> &type)
  {
    type.Named("Prefab", "The prefab recipe the entity was placed from");

    type.Field("path", &Prefab::path)
        .Describe("Virtual path of the prefab recipe");
  }
} // neon

#endif //PREFAB_HPP
