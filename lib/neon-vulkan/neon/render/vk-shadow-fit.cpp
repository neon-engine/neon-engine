#include "vk-shadow-fit.hpp"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace neon
{
  std::array<float, kMax_Shadow_Cascades> VK_ShadowFit::Splits(const float near, const float distance, const int count)
  {
    std::array<float, kMax_Shadow_Cascades> splits{};
    const int cascades = std::clamp(count, 1, kMax_Shadow_Cascades);
    const float from = std::max(near, 0.01f);
    const float to = std::max(distance, from + 0.01f);

    // halfway between even steps, which give the far slices too much, and
    // logarithmic ones, which give the near slice almost nothing
    for (int i = 0; i < cascades; i++)
    {
      const float part = static_cast<float>(i + 1) / static_cast<float>(cascades);
      const float even = from + (to - from) * part;
      const float logarithmic = from * std::pow(to / from, part);
      splits[static_cast<std::size_t>(i)] = 0.5f * (even + logarithmic);
    }
    splits[static_cast<std::size_t>(cascades - 1)] = to;
    return splits;
  }

  std::array<glm::vec3, 8> VK_ShadowFit::SliceCorners(
    const glm::mat4 &inverse_view_projection,
    const float near,
    const float far,
    const float from,
    const float to)
  {
    // the corners of the whole view at its near and its far plane, in the
    // world; a slice lies between them along straight edges, at a fraction
    // of the way that its distance is of the depth of the view
    std::array<glm::vec3, 4> at_near{};
    std::array<glm::vec3, 4> at_far{};
    constexpr std::array<glm::vec2, 4> square{glm::vec2(-1, -1), glm::vec2(1, -1), glm::vec2(1, 1), glm::vec2(-1, 1)};
    for (std::size_t i = 0; i < 4; i++)
    {
      const glm::vec4 n = inverse_view_projection * glm::vec4(square[i], -1.0f, 1.0f);
      const glm::vec4 f = inverse_view_projection * glm::vec4(square[i], 1.0f, 1.0f);
      at_near[i] = glm::vec3(n) / n.w;
      at_far[i] = glm::vec3(f) / f.w;
    }

    const float depth = std::max(far - near, 0.001f);
    const float start = std::clamp((from - near) / depth, 0.0f, 1.0f);
    const float end = std::clamp((to - near) / depth, 0.0f, 1.0f);

    std::array<glm::vec3, 8> corners{};
    for (std::size_t i = 0; i < 4; i++)
    {
      corners[i] = glm::mix(at_near[i], at_far[i], start);
      corners[i + 4] = glm::mix(at_near[i], at_far[i], end);
    }
    return corners;
  }

  glm::mat4 VK_ShadowFit::View(const glm::vec3 &direction, const glm::vec3 &centre, const float radius, const float reach)
  {
    const glm::vec3 along = glm::normalize(direction);

    // a light straight down has no sideways from y, and takes x
    const glm::vec3 up = std::abs(along.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    return glm::lookAt(centre - along * (radius + reach), centre, up);
  }

  glm::mat4 VK_ShadowFit::BoxViewProjection(
    const glm::vec3 &direction,
    const glm::vec3 &centre,
    const float radius,
    const float reach,
    const float map_size)
  {
    // The centre is at the origin of the light's plane. The box around it
    // is moved so that its corners sit on whole texels, which has the
    // shadows stay where they are while the centre moves within a texel.
    // The move is worked out from where the origin of the world lands,
    // which is what moves with the centre.
    const glm::mat4 view = View(direction, centre, radius, reach);
    const float texel = 2.0f * radius / map_size;

    const glm::vec4 origin = view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const float snap_x = std::round(origin.x / texel) * texel - origin.x;
    const float snap_y = std::round(origin.y / texel) * texel - origin.y;

    const glm::mat4 projection = glm::ortho(
      -radius - snap_x, radius - snap_x, -radius - snap_y, radius - snap_y, 0.0f, 2.0f * radius + reach);

    return projection * view;
  }

  VK_ShadowCascades VK_ShadowFit::Cascades(
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const glm::vec3 &direction,
    const float distance,
    const int count,
    const float map_size)
  {
    VK_ShadowCascades cascades;
    cascades.count = std::clamp(count, 1, kMax_Shadow_Cascades);

    // the planes of the camera, read back from its projection as glm
    // writes a perspective one
    const float a = projection[2][2];
    const float b = projection[3][2];
    const float near = std::abs(a - 1.0f) > 1e-6f ? b / (a - 1.0f) : 0.1f;
    const float far = std::abs(a + 1.0f) > 1e-6f ? b / (a + 1.0f) : 1000.0f;

    const float reach = std::max(distance, 1.0f);
    cascades.splits = Splits(near, std::min(distance, far), cascades.count);
    const glm::mat4 inverse = glm::inverse(projection * view);

    float from = near;
    for (int i = 0; i < cascades.count; i++)
    {
      const float to = cascades.splits[static_cast<std::size_t>(i)];
      const auto corners = SliceCorners(inverse, near, far, from, to);

      // the sphere around the slice: one size however the camera turns
      glm::vec3 centre(0.0f);
      for (const glm::vec3 &corner : corners) { centre += corner; }
      centre /= 8.0f;
      float radius = 0.0f;
      for (const glm::vec3 &corner : corners) { radius = std::max(radius, glm::length(corner - centre)); }
      radius = std::max(radius, 0.5f);

      cascades.view_projections[static_cast<std::size_t>(i)] =
        BoxViewProjection(direction, centre, radius, reach, map_size);
      from = to;
    }
    return cascades;
  }
} // neon
