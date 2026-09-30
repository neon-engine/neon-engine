#include "luna-vector-image-rasterizer.hpp"

#include <cstddef>
#include <cstring>
#include <format>

#include <lunasvg.h>

namespace neon
{
  struct LUNA_VectorImageRasterizer::Image
  {
    std::unique_ptr<lunasvg::Document> document;
  };

  LUNA_VectorImageRasterizer::LUNA_VectorImageRasterizer() = default;

  LUNA_VectorImageRasterizer::~LUNA_VectorImageRasterizer() = default;

  const LUNA_VectorImageRasterizer::Image *LUNA_VectorImageRasterizer::Find(const int image) const
  {
    if (image < 0 || static_cast<std::size_t>(image) >= _images.size()) { return nullptr; }

    return _images[image].get();
  }

  std::string LUNA_VectorImageRasterizer::FindFileOfAnImage(const std::string &svg)
  {
    // Every `image` and what it refers to, which is written as `href` or
    // as `xlink:href`, in single or in double quotes.
    std::size_t at = 0;

    while ((at = svg.find("<image", at)) != std::string::npos)
    {
      const std::size_t end = svg.find('>', at);
      if (end == std::string::npos) { break; }

      const std::string element = svg.substr(at, end - at);
      at = end;

      std::size_t name = 0;
      while ((name = element.find("href", name)) != std::string::npos)
      {
        std::size_t value = name + 4;
        name = value;

        while (value < element.size() && (element[value] == ' ' || element[value] == '\n' ||
                                          element[value] == '\t' || element[value] == '\r'))
        {
          value++;
        }
        if (value >= element.size() || element[value] != '=') { continue; }
        value++;

        while (value < element.size() && (element[value] == ' ' || element[value] == '\n' ||
                                          element[value] == '\t' || element[value] == '\r'))
        {
          value++;
        }
        if (value >= element.size() || (element[value] != '"' && element[value] != '\'')) { continue; }

        const char quote = element[value];
        const std::size_t closing = element.find(quote, value + 1);
        if (closing == std::string::npos) { continue; }

        std::string href = element.substr(value + 1, closing - value - 1);
        while (!href.empty() && (href.front() == ' ' || href.front() == '\n')) { href.erase(href.begin()); }

        if (href.compare(0, 5, "data:") != 0 && !href.empty()) { return href; }
      }
    }

    return "";
  }

  int LUNA_VectorImageRasterizer::Load(const std::vector<unsigned char> &file, std::string &error)
  {
    if (file.empty())
    {
      error = "the file is empty";
      return -1;
    }

    const std::string text(file.begin(), file.end());

    // LunaSVG opens the file of such an image by itself, past the file
    // system of the engine and its rules. It is not given the chance.
    if (const std::string other = FindFileOfAnImage(text); !other.empty())
    {
      error = std::format(
        "it refers to the image '{}', which is a file of its own. An image inside an SVG has to be part of "
        "the SVG, as data",
        other);
      return -1;
    }

    auto image = std::make_unique<Image>();
    image->document = lunasvg::Document::loadFromData(text.data(), text.size());

    if (image->document == nullptr)
    {
      error = "it is no SVG that can be read";
      return -1;
    }

    if (image->document->width() <= 0.0f || image->document->height() <= 0.0f)
    {
      error = "it has no size: neither a width and a height nor a viewBox say how large it is";
      return -1;
    }

    for (std::size_t i = 0; i < _images.size(); i++)
    {
      if (_images[i] == nullptr)
      {
        _images[i] = std::move(image);
        return static_cast<int>(i);
      }
    }

    _images.push_back(std::move(image));
    return static_cast<int>(_images.size()) - 1;
  }

  void LUNA_VectorImageRasterizer::Unload(const int image)
  {
    if (Find(image) != nullptr) { _images[image].reset(); }
  }

  bool LUNA_VectorImageRasterizer::GetSize(const int image, float &width, float &height)
  {
    const Image *found = Find(image);
    if (found == nullptr) { return false; }

    width = found->document->width();
    height = found->document->height();
    return true;
  }

  bool LUNA_VectorImageRasterizer::Rasterize(
    const int image,
    const int width,
    const int height,
    ImagePixels &pixels)
  {
    const Image *found = Find(image);
    if (found == nullptr || width <= 0 || height <= 0 || width > kMax_Size || height > kMax_Size) { return false; }

    // with nothing behind it, so that what the image leaves free is
    // see-through
    lunasvg::Bitmap bitmap = found->document->renderToBitmap(width, height, 0x00000000);
    if (bitmap.isNull() || bitmap.width() != width || bitmap.height() != height) { return false; }

    // from blue, green, red, alpha with alpha multiplied in to what the
    // engine keeps
    bitmap.convertToRGBA();

    pixels.width = width;
    pixels.height = height;
    pixels.pixels.resize(static_cast<std::size_t>(width) * height * 4);

    const int stride = bitmap.stride();
    for (int row = 0; row < height; row++)
    {
      std::memcpy(
        pixels.pixels.data() + static_cast<std::size_t>(row) * width * 4,
        bitmap.data() + static_cast<std::size_t>(row) * stride,
        static_cast<std::size_t>(width) * 4);
    }

    return true;
  }
} // neon
