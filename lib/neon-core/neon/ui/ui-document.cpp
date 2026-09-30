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
} // neon
