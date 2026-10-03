#include "vk-shadow-casting.hpp"

#include <algorithm>

namespace neon
{
  std::vector<VK_ShadowCasting::Cast> VK_ShadowCasting::Plan(const std::vector<Caster> &casters)
  {
    std::vector<Cast> casts(casters.size());
    if (casters.empty()) { return casts; }

    const bool alike = std::ranges::all_of(casters, [&casters](const Caster &caster)
    {
      return caster.casts && caster.pipeline != VK_NULL_HANDLE && caster.pipeline == casters.front().pipeline;
    });

    if (alike)
    {
      // the first draw casts the whole model, the others nothing
      casts.front() = {.casts = true, .model_material = -1};
      return casts;
    }

    for (std::size_t i = 0; i < casters.size(); i++)
    {
      casts[i] = {
        .casts = casters[i].casts && casters[i].pipeline != VK_NULL_HANDLE,
        .model_material = casters[i].model_material};
    }
    return casts;
  }
} // neon
