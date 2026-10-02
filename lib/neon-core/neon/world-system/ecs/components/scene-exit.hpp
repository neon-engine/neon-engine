#ifndef SCENE_EXIT_HPP
#define SCENE_EXIT_HPP

#include <string>

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Makes the Trigger of the entity the way out of the scene: when a body
  /// is inside it, the world changes to the scene it names, as the door at
  /// the end of a level does. Without a Trigger on the entity it does
  /// nothing. The change happens at the start of the next frame, and the
  /// exit itself goes with the old scene, unless it is Persistent.
  struct SceneExit
  {
    /// Virtual path of the scene to change to.
    std::string scene;
  };

  inline void Describe(TypeBuilder<SceneExit> &type)
  {
    type.Named("SceneExit", "Changes to a scene when a body enters the Trigger of the entity");

    type.Field("scene", &SceneExit::scene)
        .Describe("Virtual path of the scene to change to");
  }
} // neon

#endif //SCENE_EXIT_HPP
