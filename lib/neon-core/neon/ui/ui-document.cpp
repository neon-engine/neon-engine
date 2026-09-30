#include "ui-document.hpp"

#include <algorithm>

namespace neon
{
  float UiDocument::ScaleFor(const int frame_width, const int frame_height) const
  {
    if (reference_width <= 0.0f || reference_height <= 0.0f) { return 1.0f; }

    const float by_width = static_cast<float>(frame_width) / reference_width;
    const float by_height = static_cast<float>(frame_height) / reference_height;

    float scale = 1.0f;
    switch (scale_mode)
    {
      case UiScaleMode::Fit:
        scale = std::min(by_width, by_height);
        break;
      case UiScaleMode::Width:
        scale = by_width;
        break;
      case UiScaleMode::Height:
        scale = by_height;
        break;
      default:
        break;
    }

    // a frame without a size draws nothing, and must not divide by zero
    return scale > 0.0f ? scale : 1.0f;
  }

  float UiDocument::ScaleFor(
    const float point_width,
    const float point_height,
    const float density,
    const float user_scale) const
  {
    const float by_width = reference_width > 0.0f ? point_width / reference_width : 1.0f;
    const float by_height = reference_height > 0.0f ? point_height / reference_height : 1.0f;

    float asked = 1.0f;
    switch (scale_mode)
    {
      case UiScaleMode::Fit:
        asked = std::min(by_width, by_height);
        break;
      case UiScaleMode::Width:
        asked = by_width;
        break;
      case UiScaleMode::Height:
        asked = by_height;
        break;
      default:
        break;
    }

    const float scale = (density > 0.0f ? density : 1.0f) * (user_scale > 0.0f ? user_scale : 1.0f) * asked;
    return scale > 0.0f ? scale : 1.0f;
  }

  const DataValue *UiDocument::FindTemplate(const std::string &template_name) const
  {
    for (const auto &[name, description] : templates)
    {
      if (name == template_name) { return &description; }
    }
    return nullptr;
  }
} // neon
