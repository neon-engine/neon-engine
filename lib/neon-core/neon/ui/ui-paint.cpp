#include "ui-paint.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  void UiGradient::Settle()
  {
    if (stops.empty()) { return; }

    // the first and the last are at the ends unless the file says
    // otherwise
    if (!stops.front().has_position)
    {
      stops.front().position = 0.0f;
      stops.front().has_position = true;
    }

    if (!stops.back().has_position)
    {
      stops.back().position = 1.0f;
      stops.back().has_position = true;
    }

    // none lies in front of the one before it
    float furthest = stops.front().position;
    for (auto &stop : stops)
    {
      if (!stop.has_position) { continue; }

      stop.position = std::max(stop.position, furthest);
      furthest = stop.position;
    }

    // what has no place lies evenly between its neighbors that have one
    for (std::size_t i = 1; i + 1 < stops.size(); i++)
    {
      if (stops[i].has_position) { continue; }

      std::size_t next = i;
      while (!stops[next].has_position) { next++; }

      const float from = stops[i - 1].position;
      const float to = stops[next].position;
      const auto steps = static_cast<float>(next - i + 1);

      for (std::size_t k = i; k < next; k++)
      {
        stops[k].position = from + (to - from) * static_cast<float>(k - i + 1) / steps;
        stops[k].has_position = true;
      }
    }
  }

  Color UiGradient::At(const float position) const
  {
    if (stops.empty()) { return {0.0f, 0.0f, 0.0f, 0.0f}; }
    if (position <= stops.front().position) { return stops.front().color; }

    for (std::size_t i = 1; i < stops.size(); i++)
    {
      if (position > stops[i].position) { continue; }

      const UiGradientStop &from = stops[i - 1];
      const UiGradientStop &to = stops[i];
      const float length = to.position - from.position;
      const float part = length > 0.0f ? (position - from.position) / length : 1.0f;

      // Blended with alpha multiplied in, as CSS does, so that a color
      // that is see-through does not shine through its neighbor.
      const float alpha = from.color.a + (to.color.a - from.color.a) * part;
      const auto blend = [&](const float a, const float b)
      {
        const float value = a * from.color.a + (b * to.color.a - a * from.color.a) * part;
        return alpha > 0.0f ? value / alpha : 0.0f;
      };

      return {
        blend(from.color.r, to.color.r),
        blend(from.color.g, to.color.g),
        blend(from.color.b, to.color.b),
        alpha
      };
    }

    return stops.back().color;
  }

  UiMatrix ToMatrix(
    const std::vector<UiTransformStep> &steps,
    const float width,
    const float height,
    const float origin_x,
    const float origin_y)
  {
    const auto resolve = [](const LayoutLength &length, const float whole)
    {
      return length.unit == LayoutLength::Unit::Percent ? length.value / 100.0f * whole : length.value;
    };

    UiMatrix matrix;

    // from the last step to the first, each after what follows it
    for (std::size_t i = steps.size(); i > 0; i--)
    {
      const UiTransformStep &step = steps[i - 1];
      UiMatrix each;

      switch (step.kind)
      {
        case UiTransformStep::Kind::Translate:
          each.e = resolve(step.x, width);
          each.f = resolve(step.y, height);
          break;
        case UiTransformStep::Kind::Rotate:
        {
          const float radians = step.angle * 3.14159265358979f / 180.0f;
          each.a = std::cos(radians);
          each.b = std::sin(radians);
          each.c = -each.b;
          each.d = each.a;
          break;
        }
        case UiTransformStep::Kind::Scale:
          each.a = step.scale_x;
          each.d = step.scale_y;
          break;
      }

      matrix = each.After(matrix);
    }

    // around the origin: there, what the steps say, and back
    UiMatrix to_origin;
    to_origin.e = -origin_x;
    to_origin.f = -origin_y;

    UiMatrix back;
    back.e = origin_x;
    back.f = origin_y;

    return back.After(matrix.After(to_origin));
  }

  std::size_t ChooseImageSource(const std::vector<UiImageSource> &sources, const float scale)
  {
    std::size_t chosen = 0;
    bool is_enough = false;

    for (std::size_t i = 0; i < sources.size(); i++)
    {
      const float density = sources[i].scale;
      const float current = sources[chosen].scale;

      // a little less than what is asked for counts as enough, so that a
      // scale of 1.98 takes the image that was made for 2
      const bool suffices = density >= scale - 0.05f;

      if (i == 0)
      {
        is_enough = suffices;
        continue;
      }

      if (suffices && (!is_enough || density < current))
      {
        chosen = i;
        is_enough = true;
      } else if (!suffices && !is_enough && density > current)
      {
        chosen = i;
      }
    }

    return chosen;
  }
} // neon
