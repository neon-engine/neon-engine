#include "vk-draw-queue.hpp"

#include <algorithm>
#include <tuple>

namespace neon
{
  void VK_DrawQueue::Settle()
  {
    _batches.clear();

    // the see-through draws keep the order they came in, behind the opaque
    // ones; the opaque ones are ordered by what they are drawn with, and
    // the nearest first among those alike
    std::stable_partition(_draws.begin(), _draws.end(), [](const VK_Draw &draw) { return !draw.see_through; });
    const auto see_through = std::find_if(_draws.begin(), _draws.end(), [](const VK_Draw &draw) { return draw.see_through; });

    std::stable_sort(_draws.begin(), see_through, [](const VK_Draw &a, const VK_Draw &b)
    {
      return std::tie(a.pipeline, a.set, a.model_id, a.model_material, a.distance) <
             std::tie(b.pipeline, b.set, b.model_id, b.model_material, b.distance);
    });

    for (auto it = _draws.begin(); it != see_through; ++it)
    {
      const auto index = static_cast<uint32_t>(it - _draws.begin());
      if (!_batches.empty())
      {
        VK_DrawBatch &last = _batches.back();
        if (last.pipeline == it->pipeline && last.set == it->set && last.model_id == it->model_id &&
            last.model_material == it->model_material && last.scene_offset == it->scene_offset)
        {
          last.instances++;
          continue;
        }
      }
      _batches.push_back({
        .pipeline = it->pipeline,
        .set = it->set,
        .scene_offset = it->scene_offset,
        .model_id = it->model_id,
        .model_material = it->model_material,
        .first_instance = index,
        .instances = 1
      });
    }

    // The pass that draws the shadow map writes depth alone, so which
    // material an object is drawn with does not part its draws there. The
    // casters are put in an order of their own, by the pipeline of the
    // pass, the model, and the meshes, and those alike are one call. Their
    // objects are written into the buffer of the frame once more in that
    // order, see ShadowOrder(), so that a batch has its objects side by
    // side wherever they stand among the draws of the scene.
    _shadow_batches.clear();
    _shadow_order.clear();
    for (auto it = _draws.begin(); it != see_through; ++it)
    {
      if (it->casts_shadow && it->shadow_pipeline != VK_NULL_HANDLE)
      {
        _shadow_order.push_back(static_cast<uint32_t>(it - _draws.begin()));
      }
    }
    std::ranges::stable_sort(_shadow_order, [this](const uint32_t a, const uint32_t b)
    {
      const VK_Draw &x = _draws[a];
      const VK_Draw &y = _draws[b];
      return std::tie(x.shadow_pipeline, x.scene_offset, x.model_id, x.shadow_material) <
             std::tie(y.shadow_pipeline, y.scene_offset, y.model_id, y.shadow_material);
    });
    for (std::size_t place = 0; place < _shadow_order.size(); place++)
    {
      const VK_Draw &draw = _draws[_shadow_order[place]];
      if (!_shadow_batches.empty())
      {
        VK_ShadowBatch &last = _shadow_batches.back();
        if (last.pipeline == draw.shadow_pipeline && last.scene_offset == draw.scene_offset &&
            last.model_id == draw.model_id && last.model_material == draw.shadow_material)
        {
          last.instances++;
          continue;
        }
      }
      _shadow_batches.push_back({
        .pipeline = draw.shadow_pipeline,
        .set = draw.set,
        .scene_offset = draw.scene_offset,
        .model_id = draw.model_id,
        .model_material = draw.shadow_material,
        .first_instance = static_cast<uint32_t>(place),
        .instances = 1
      });
    }
  }

  std::size_t VK_DrawQueue::SeeThroughStart() const
  {
    const auto it = std::find_if(_draws.begin(), _draws.end(), [](const VK_Draw &draw) { return draw.see_through; });
    return static_cast<std::size_t>(it - _draws.begin());
  }

  void VK_DrawQueue::Clear()
  {
    _draws.clear();
    _batches.clear();
    _shadow_batches.clear();
    _shadow_order.clear();
  }
} // neon
