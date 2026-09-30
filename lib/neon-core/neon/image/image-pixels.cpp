#include "image-pixels.hpp"

#include <algorithm>
#include <cstddef>

namespace neon
{
  void PremultiplyAlpha(std::vector<unsigned char> &pixels)
  {
    for (std::size_t i = 0; i + 3 < pixels.size(); i += 4)
    {
      const unsigned int alpha = pixels[i + 3];

      for (std::size_t channel = 0; channel < 3; channel++)
      {
        pixels[i + channel] = static_cast<unsigned char>((pixels[i + channel] * alpha + 127) / 255);
      }
    }
  }

  std::vector<ImagePixels> MakeSmallerCopies(const ImagePixels &image)
  {
    std::vector<ImagePixels> copies;
    if (image.IsEmpty() || image.pixels.size() != static_cast<std::size_t>(image.width) * image.height * 4)
    {
      return copies;
    }

    copies.push_back(image);

    while (copies.back().width > 1 || copies.back().height > 1)
    {
      const ImagePixels &from = copies.back();

      ImagePixels copy;
      copy.width = std::max(from.width / 2, 1);
      copy.height = std::max(from.height / 2, 1);
      copy.pixels.resize(static_cast<std::size_t>(copy.width) * copy.height * 4);

      for (int row = 0; row < copy.height; row++)
      {
        // The rows and columns a pixel stands for. An image with an odd
        // side has one more than fits, which the last pixel takes in.
        const int first_row = row * from.height / copy.height;
        const int end_row = std::max((row + 1) * from.height / copy.height, first_row + 1);

        for (int column = 0; column < copy.width; column++)
        {
          const int first_column = column * from.width / copy.width;
          const int end_column = std::max((column + 1) * from.width / copy.width, first_column + 1);

          unsigned int sums[4] = {0, 0, 0, 0};
          unsigned int count = 0;

          for (int y = first_row; y < end_row; y++)
          {
            for (int x = first_column; x < end_column; x++)
            {
              const std::size_t at = (static_cast<std::size_t>(y) * from.width + x) * 4;
              for (std::size_t channel = 0; channel < 4; channel++) { sums[channel] += from.pixels[at + channel]; }
              count++;
            }
          }

          const std::size_t to = (static_cast<std::size_t>(row) * copy.width + column) * 4;
          for (std::size_t channel = 0; channel < 4; channel++)
          {
            copy.pixels[to + channel] = static_cast<unsigned char>((sums[channel] + count / 2) / count);
          }
        }
      }

      copies.push_back(std::move(copy));
    }

    return copies;
  }

  void CopyPixels(
    const ImagePixels &from,
    const int from_x,
    const int from_y,
    const int width,
    const int height,
    ImagePixels &target,
    const int target_x,
    const int target_y)
  {
    for (int row = 0; row < height; row++)
    {
      const int source_row = from_y + row;
      const int target_row = target_y + row;
      if (source_row < 0 || source_row >= from.height || target_row < 0 || target_row >= target.height) { continue; }

      for (int column = 0; column < width; column++)
      {
        const int source_column = from_x + column;
        const int target_column = target_x + column;
        if (source_column < 0 || source_column >= from.width ||
            target_column < 0 || target_column >= target.width)
        {
          continue;
        }

        const std::size_t source = (static_cast<std::size_t>(source_row) * from.width + source_column) * 4;
        const std::size_t to = (static_cast<std::size_t>(target_row) * target.width + target_column) * 4;

        const unsigned int alpha = from.pixels[source + 3];

        for (std::size_t channel = 0; channel < 4; channel++)
        {
          const unsigned int under = target.pixels[to + channel];
          const unsigned int value = from.pixels[source + channel] + (under * (255 - alpha) + 127) / 255;
          target.pixels[to + channel] = static_cast<unsigned char>(std::min(value, 255u));
        }
      }
    }
  }
} // neon
