#include "color-space.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace neon
{
  // Helpers of color-space.cpp, for this file alone.
  namespace
  {
    std::array<float, 256> MakeByteTable()
    {
      std::array<float, 256> table{};
      for (std::size_t i = 0; i < table.size(); i++)
      {
        table[i] = SrgbToLinear(static_cast<float>(i) / 255.0f);
      }
      return table;
    }
  }

  // The curve of the sRGB standard: a short straight part near black, so
  // that the curve does not get infinitely steep there, and a power above.
  float SrgbToLinear(const float value)
  {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped <= 0.04045f ? clamped / 12.92f : std::pow((clamped + 0.055f) / 1.055f, 2.4f);
  }

  float LinearToSrgb(const float value)
  {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped <= 0.0031308f ? clamped * 12.92f : 1.055f * std::pow(clamped, 1.0f / 2.4f) - 0.055f;
  }

  Color SrgbToLinear(const Color &color)
  {
    return {SrgbToLinear(color.r), SrgbToLinear(color.g), SrgbToLinear(color.b), color.a};
  }

  float SrgbByteToLinear(const unsigned char value)
  {
    static const std::array<float, 256> table = MakeByteTable();
    return table[value];
  }

  unsigned char LinearToSrgbByte(const float value)
  {
    return static_cast<unsigned char>(std::lround(LinearToSrgb(value) * 255.0f));
  }
} // neon
