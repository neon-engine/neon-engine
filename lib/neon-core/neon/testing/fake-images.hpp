#ifndef FAKE_IMAGES_HPP
#define FAKE_IMAGES_HPP

#include <cstddef>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

#include <neon/image/image-decoder.hpp>
#include <neon/image/vector-image-rasterizer.hpp>

namespace neon::testing
{
  /// Reads images that are written as text, so that a test can say what an
  /// image holds without a file of a real format:
  ///
  ///     image 64 32
  ///
  /// is an image of 64 by 32 pixels, white and covering everything.
  /// Anything else is no image.
  class FakeImageDecoder final : public ImageDecoder
  {
  public:
    /// How often an image was read, which includes what failed.
    std::size_t decoded = 0;

    static std::string AnImage(const int width, const int height)
    {
      return "image " + std::to_string(width) + " " + std::to_string(height);
    }

    bool Decode(const std::vector<unsigned char> &file, ImagePixels &image, std::string &error) override
    {
      decoded++;

      const std::string text(file.begin(), file.end());
      int width = 0;
      int height = 0;

      if (text.rfind("image ", 0) != 0 || std::sscanf(text.c_str(), "image %d %d", &width, &height) != 2 ||
          width <= 0 || height <= 0)
      {
        error = "it is no image of the tests";
        return false;
      }

      image.width = width;
      image.height = height;
      image.pixels.assign(static_cast<std::size_t>(width) * height * 4, 255);
      return true;
    }
  };

  /// Draws images of shapes that are written as text:
  ///
  ///     shapes 24 24
  ///
  /// is an image that says it is 24 by 24. It keeps at which sizes it was
  /// drawn.
  class FakeVectorImageRasterizer final : public VectorImageRasterizer
  {
    struct Image
    {
      float width = 0.0f;
      float height = 0.0f;
      bool is_loaded = false;
    };

    std::vector<Image> _images;

  public:
    /// The sizes that were drawn, in the order they were asked for.
    std::vector<std::pair<int, int>> drawn;

    /// How often an image was read.
    std::size_t loaded = 0;

    /// Makes Rasterize() fail.
    bool refuses = false;

    static std::string Shapes(const int width, const int height)
    {
      return "shapes " + std::to_string(width) + " " + std::to_string(height);
    }

    int Load(const std::vector<unsigned char> &file, std::string &error) override
    {
      loaded++;

      const std::string text(file.begin(), file.end());
      int width = 0;
      int height = 0;

      if (std::sscanf(text.c_str(), "shapes %d %d", &width, &height) != 2 || width <= 0 || height <= 0)
      {
        error = "it is no image of shapes of the tests";
        return -1;
      }

      _images.push_back({static_cast<float>(width), static_cast<float>(height), true});
      return static_cast<int>(_images.size()) - 1;
    }

    void Unload(const int image) override
    {
      if (image >= 0 && static_cast<std::size_t>(image) < _images.size()) { _images[image].is_loaded = false; }
    }

    [[nodiscard]] bool IsLoaded(const int image) const
    {
      return image >= 0 && static_cast<std::size_t>(image) < _images.size() && _images[image].is_loaded;
    }

    bool GetSize(const int image, float &width, float &height) override
    {
      if (!IsLoaded(image)) { return false; }

      width = _images[image].width;
      height = _images[image].height;
      return true;
    }

    bool Rasterize(const int image, const int width, const int height, ImagePixels &pixels) override
    {
      if (refuses || !IsLoaded(image) || width <= 0 || height <= 0) { return false; }

      drawn.emplace_back(width, height);

      pixels.width = width;
      pixels.height = height;
      pixels.pixels.assign(static_cast<std::size_t>(width) * height * 4, 255);
      return true;
    }
  };
} // neon::testing

#endif //FAKE_IMAGES_HPP
