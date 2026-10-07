#include "image-halving.hpp"

#include <algorithm>
#include <cmath>

#include <neon/common/color-space.hpp>

namespace neon
{
  ImagePixels HalveImage(const ImagePixels &image, const bool is_color)
  {
    if (image.IsEmpty()) { return image; }

    ImagePixels half;
    half.width = std::max(image.width / 2, 1);
    half.height = std::max(image.height / 2, 1);
    half.pixels.resize(static_cast<std::size_t>(half.width) * half.height * 4);

    // the four pixels a pixel of the half stands for, those past the edge
    // of a side of 1 being the one there is
    const auto at = [&image](const int x, const int y, const int channel)
    {
      const int clamped_x = std::min(x, image.width - 1);
      const int clamped_y = std::min(y, image.height - 1);
      return image.pixels[(static_cast<std::size_t>(clamped_y) * image.width + clamped_x) * 4 + channel];
    };

    for (int y = 0; y < half.height; y++)
    {
      for (int x = 0; x < half.width; x++)
      {
        for (int channel = 0; channel < 4; channel++)
        {
          const unsigned char a = at(2 * x, 2 * y, channel);
          const unsigned char b = at(2 * x + 1, 2 * y, channel);
          const unsigned char c = at(2 * x, 2 * y + 1, channel);
          const unsigned char d = at(2 * x + 1, 2 * y + 1, channel);

          unsigned char average;
          if (is_color && channel < 3)
          {
            const float light =
              (SrgbByteToLinear(a) + SrgbByteToLinear(b) + SrgbByteToLinear(c) + SrgbByteToLinear(d)) / 4.0f;
            average = LinearToSrgbByte(light);
          } else
          {
            average = static_cast<unsigned char>((a + b + c + d + 2) / 4);
          }

          half.pixels[(static_cast<std::size_t>(y) * half.width + x) * 4 + channel] = average;
        }
      }
    }
    return half;
  }
} // neon
