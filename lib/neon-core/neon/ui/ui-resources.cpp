#include "ui-resources.hpp"

#include <algorithm>
#include <cstdlib>

namespace neon
{
  UiResources::UiResources(
    Render2DContext *renderer,
    FontRasterizer *rasterizer,
    FileSystemContext *file_system,
    const std::shared_ptr<Logger> &logger)
  {
    _renderer = renderer;
    _rasterizer = rasterizer;
    _file_system = file_system;
    _logger = logger;
  }

  void UiResources::AddFace(const std::string &family, const int weight, const std::string &path)
  {
    for (auto &face : _faces)
    {
      if (face.family != family || face.weight != weight) { continue; }

      // what was drawn with stays, since the sizes that were made refer
      // to it
      if (!face.is_tried) { face.path = path; }
      return;
    }

    _faces.push_back({family, weight, path, false, -1});
  }

  bool UiResources::HasFamily(const std::string &family) const
  {
    return std::ranges::any_of(_faces, [&family](const Face &face) { return face.family == family; });
  }

  UiResources::Face *UiResources::FindFace(const std::string &family, const int weight)
  {
    Face *nearest = nullptr;

    for (auto &face : _faces)
    {
      if (face.family != family) { continue; }

      if (nearest == nullptr || std::abs(face.weight - weight) < std::abs(nearest->weight - weight))
      {
        nearest = &face;
      }
    }

    return nearest;
  }

  const UiFont *UiResources::GetFont(const std::string &family, const int weight, const int pixel_size)
  {
    Face *face = FindFace(family, weight);

    if (face == nullptr)
    {
      if (std::ranges::find(_unknown_families, family) == _unknown_families.end())
      {
        _unknown_families.push_back(family);
        _logger->Error("The font family '{}' is not known, text that asks for it is not drawn", family);
      }
      return nullptr;
    }

    if (!face->is_tried)
    {
      face->is_tried = true;

      if (std::vector<unsigned char> file; !_file_system->ReadBytes(face->path, file))
      {
        _logger->Error("The font {} cannot be read", face->path);
      } else if (face->font = _rasterizer->LoadFont(file); face->font < 0)
      {
        _logger->Error("{} is not a font that can be used", face->path);
      } else
      {
        _logger->Info("Loaded the font {}", face->path);
      }
    }

    if (face->font < 0 || pixel_size <= 0) { return nullptr; }

    const auto key = std::make_pair(static_cast<std::size_t>(face - _faces.data()), pixel_size);
    if (const auto found = _fonts.find(key); found != _fonts.end()) { return found->second.get(); }

    auto font = std::make_unique<UiFont>();
    std::string error;

    if (!font->atlas.Build(
      *_rasterizer, face->font, static_cast<float>(pixel_size), FontAtlas::DefaultCharacters(), error))
    {
      _logger->Error("The font {} cannot be drawn at {} pixels: {}", face->path, pixel_size, error);
      font.reset();
    } else
    {
      font->texture = _renderer->CreateTexture(
        font->atlas.GetWidth(), font->atlas.GetHeight(), font->atlas.GetPixels());

      if (font->texture == No_Texture)
      {
        _logger->Error(
          "The renderer took no texture for the font {} at {} pixels", face->path, pixel_size);
        font.reset();
      }
    }

    // what failed is kept as well, so that it is not tried again
    return (_fonts[key] = std::move(font)).get();
  }

  UiImage UiResources::GetImage(const std::string &path)
  {
    if (const auto found = _images.find(path); found != _images.end()) { return found->second; }

    UiImage image;
    image.texture = _renderer->LoadTexture(path);

    if (image.texture == No_Texture)
    {
      _logger->Error("The image {} cannot be used, what asks for it is drawn without it", path);
    } else if (!_renderer->GetTextureSize(image.texture, image.width, image.height))
    {
      image.width = 0;
      image.height = 0;
    }

    _images[path] = image;
    return image;
  }

  void UiResources::CleanUp()
  {
    for (const auto &[key, font] : _fonts)
    {
      if (font != nullptr && font->texture != No_Texture) { _renderer->DestroyTexture(font->texture); }
    }
    _fonts.clear();

    for (const auto &[path, image] : _images)
    {
      if (image.texture != No_Texture) { _renderer->DestroyTexture(image.texture); }
    }
    _images.clear();

    for (auto &face : _faces)
    {
      if (face.font >= 0) { _rasterizer->UnloadFont(face.font); }
      face.font = -1;
      face.is_tried = false;
    }

    _unknown_families.clear();
  }
} // neon
