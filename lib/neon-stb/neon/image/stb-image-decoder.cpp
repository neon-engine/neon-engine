#include "stb-image-decoder.hpp"

#include <cstddef>
#include <format>

// kept private to this file, so that another library can carry its own copy
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include <stb_image.h>

namespace neon
{
  bool STB_ImageDecoder::Decode(const std::vector<unsigned char> &file, ImagePixels &image, std::string &error)
  {
    if (file.empty())
    {
      error = "the file is empty";
      return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    // the size is looked at before anything is set aside for the pixels
    if (stbi_info_from_memory(file.data(), static_cast<int>(file.size()), &width, &height, &channels) == 0)
    {
      error = "it is none of PNG, JPEG, TGA, BMP, PSD, and GIF, or it is damaged";
      return false;
    }

    if (width <= 0 || height <= 0 || width > kMax_Size || height > kMax_Size)
    {
      constexpr int limit = kMax_Size;
      error = std::format("it is {} by {}, where up to {} by {} is read", width, height, limit, limit);
      return false;
    }

    unsigned char *pixels = stbi_load_from_memory(
      file.data(),
      static_cast<int>(file.size()),
      &width,
      &height,
      &channels,
      STBI_rgb_alpha);

    if (pixels == nullptr)
    {
      const char *reason = stbi_failure_reason();
      error = std::format("it cannot be read: {}", reason != nullptr ? reason : "no reason is given");
      return false;
    }

    image.width = width;
    image.height = height;
    image.pixels.assign(pixels, pixels + static_cast<std::size_t>(width) * height * 4);

    stbi_image_free(pixels);
    return true;
  }
} // neon
