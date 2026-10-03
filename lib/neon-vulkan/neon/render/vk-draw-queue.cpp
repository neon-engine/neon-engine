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
            last.model_material == it->model_material &&
            last.shadow_pipeline == it->shadow_pipeline && last.casts_shadow == it->casts_shadow &&
            last.scene_offset == it->scene_offset)
        {
          last.instances++;
          continue;
        }
      }
      _batches.push_back({
        .pipeline = it->pipeline,
        .shadow_pipeline = it->shadow_pipeline,
        .set = it->set,
        .scene_offset = it->scene_offset,
        .model_id = it->model_id,
        .model_material = it->model_material,
        .first_instance = index,
        .instances = 1,
        .casts_shadow = it->casts_shadow
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
  }
} // neon
