#include "transform-propagation.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <neon/common/transform.hpp>

namespace neon
{
  void TransformPropagation::Initialize(EntityStore &store)
  {
    _query = store.Query<Transform>(QueryOrder::ParentsFirst);
  }

  void TransformPropagation::Update(EntityStore &store, const double delta_time)
  {
    store.Each(_query, [&](const EntityBlock &block)
    {
      auto parent_matrix = glm::mat4{1.0f};
      if (block.parent != No_Entity)
      {
        // parents come first, so this is the result of the current frame
        parent_matrix = store.Get<Transform>(block.parent)->world_coordinates;
      }

      auto *transforms = block.Column<Transform>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &transform = transforms[i];

        const auto position = translate(glm::mat4{1.0f}, transform.position);
        const auto rotation = mat4_cast(transform.rotation.GetQuaternion());
        const auto scale = glm::scale(glm::mat4{1.0f}, transform.scale);

        transform.world_coordinates = parent_matrix * position * rotation * scale;
      }
    });
  }
} // neon
