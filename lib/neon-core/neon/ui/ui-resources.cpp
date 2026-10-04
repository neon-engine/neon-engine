#include "ui-resources.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <format>

#include <neon/data/data-reader.hpp>
#include <neon/render/texture-source.hpp>

#include "ui-style.hpp"
#include "ui-values.hpp"

namespace neon
{
  std::size_t UiResources::ReleaseFonts()
  {
    std::size_t destroyed = 0;

    for (const auto &[key, font] : _fonts)
    {
      if (font != nullptr && font->texture != No_Texture)
      {
        _renderer->DestroyTexture(font->texture);
        destroyed++;
      }
    }
    _fonts.clear();

    // the fonts whose glyphs are drawn on demand, page by page
    for (const auto &[key, font] : _glyph_fonts)
    {
      for (const int texture : font->textures)
      {
        if (texture != No_Texture)
        {
          _renderer->DestroyTexture(texture);
          destroyed++;
        }
      }
    }
    _glyph_fonts.clear();

    // what refers to the glyph fonts goes with them
    _text_fonts.clear();
    _fonts_revision++;

    return destroyed;
  }

  std::size_t UiResources::GetFontCount() const
  {
    std::size_t count = 0;
    for (const auto &[key, font] : _fonts)
    {
      if (font != nullptr && font->texture != No_Texture) { count++; }
    }

    for (const auto &[key, font] : _glyph_fonts)
    {
      const bool has_texture = std::ranges::any_of(font->textures, [](const int texture)
      {
        return texture != No_Texture;
      });
      if (has_texture) { count++; }
    }
    return count;
  }

  bool UiResources::IsSettling() const
  {
    if (_waits_for_surface) { return true; }

    return std::ranges::any_of(_vector_images, [](const auto &entry)
    {
      // a size was asked for that is not drawn yet
      return entry.second.wanted_for > 0;
    });
  }

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
    _fonts_revision++;
    _text_fonts.clear();

    for (auto &face : _faces)
    {
      if (face.family != family || face.weight != weight) { continue; }

      // what was drawn with stays, since the sizes that were made refer
      // to it
      if (!face.is_tried) { face.path = path; }
      return;
    }

    Face face;
    face.family = family;
    face.weight = weight;
    face.path = path;
    _faces.push_back(face);
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

    if (!Load(*face) || pixel_size <= 0) { return nullptr; }

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

  bool UiResources::Load(Face &face)
  {
    if (!face.is_tried)
    {
      face.is_tried = true;

      if (std::vector<unsigned char> file; !_file_system->ReadBytes(face.path, file))
      {
        _logger->Error("The font {} cannot be read", face.path);
      } else if (face.font = _rasterizer->LoadFont(file); face.font < 0)
      {
        _logger->Error("{} is not a font that can be used", face.path);
      } else
      {
        _logger->Info("Loaded the font {}", face.path);

        if (_shaper != nullptr)
        {
          face.shaper_font = _shaper->LoadFont(file);

          if (face.shaper_font < 0)
          {
            _logger->Warn("The font {} cannot be shaped, its text is drawn character by character", face.path);
          }
        }
      }
    }

    return face.font >= 0;
  }

  void UiResources::ReportUnknown(const std::string &family)
  {
    if (std::ranges::find(_unknown_families, family) != _unknown_families.end()) { return; }

    _unknown_families.push_back(family);
    _logger->Error("The font family '{}' is not known, text that asks for it is not drawn", family);
  }

  void UiResources::SetTextShaper(TextShaper *shaper)
  {
    _shaper = shaper;
    _fonts_revision++;
    _text_fonts.clear();
  }

  std::uint64_t UiResources::GetFontsRevision() const
  {
    return _fonts_revision;
  }

  void UiResources::AddFace(
    const std::string &family,
    const int weight,
    const std::string &path,
    const UiFaceOptions &options)
  {
    _fonts_revision++;
    _text_fonts.clear();

    for (auto &face : _faces)
    {
      if (face.family != family || face.weight != weight || face.options.is_italic != options.is_italic)
      {
        continue;
      }

      if (!face.is_tried)
      {
        face.path = path;
        face.options = options;
      }
      return;
    }

    Face face;
    face.family = family;
    face.weight = weight;
    face.path = path;
    face.options = options;
    _faces.push_back(face);
  }

  UiResources::Face *UiResources::FindFace(const std::string &family, const int weight, const bool is_italic)
  {
    Face *nearest = nullptr;

    // the style counts for more than the weight
    const auto distance = [weight, is_italic](const Face &face)
    {
      return std::abs(face.weight - weight) + (face.options.is_italic == is_italic ? 0 : 10000);
    };

    for (auto &face : _faces)
    {
      if (face.family != family) { continue; }
      if (nearest == nullptr || distance(face) < distance(*nearest)) { nearest = &face; }
    }

    return nearest;
  }

  const UiTextFonts *UiResources::GetTextFonts(
    const std::string &families,
    const int weight,
    const bool is_italic,
    const int pixel_size)
  {
    const TextFontsKey key{families, weight, is_italic, pixel_size};
    if (const auto found = _text_fonts.find(key); found != _text_fonts.end()) { return found->second.get(); }

    auto fonts = std::make_unique<UiTextFonts>();

    std::size_t start = 0;
    while (start <= families.size() && pixel_size > 0)
    {
      std::size_t end = families.find(',', start);
      if (end == std::string::npos) { end = families.size(); }

      // without the spaces around it and the quotes CSS allows
      std::string family = families.substr(start, end - start);
      const auto is_around = [](const char character)
      {
        return character == ' ' || character == '\t' || character == '"' || character == '\'';
      };
      while (!family.empty() && is_around(family.back())) { family.pop_back(); }
      while (!family.empty() && is_around(family.front())) { family.erase(family.begin()); }

      start = end + 1;
      if (family.empty()) { continue; }

      Face *face = FindFace(family, weight, is_italic);
      if (face == nullptr)
      {
        ReportUnknown(family);
        continue;
      }

      if (!Load(*face)) { continue; }

      const bool is_distance_field = face->options.rendering == GlyphRendering::DistanceField;
      const int atlas_size = is_distance_field ? kDistance_Field_Size : pixel_size;

      // What a family does not have is made from what it has: a bold by
      // making the outline thicker, an italic by leaning it.
      const bool is_made_bold = weight >= 600 && face->weight < 600;
      const bool is_made_italic = is_italic && !face->options.is_italic;

      GlyphFontKey glyph_key;
      glyph_key.face = static_cast<std::size_t>(face - _faces.data());
      glyph_key.pixel_size = atlas_size;
      glyph_key.embolden = is_made_bold ? 1 : 0;
      glyph_key.slant = is_made_italic ? 1 : 0;

      auto &glyph_font = _glyph_fonts[glyph_key];
      if (glyph_font == nullptr)
      {
        glyph_font = std::make_unique<UiGlyphFont>();
        glyph_font->path = face->path;

        GlyphAtlas::Settings settings;
        settings.rasterizer = _rasterizer;
        settings.font = face->font;
        settings.pixel_size = static_cast<float>(atlas_size);
        settings.rendering = face->options.rendering;
        settings.embolden = is_made_bold ? static_cast<float>(atlas_size) / 24.0f : 0.0f;
        settings.slant = is_made_italic ? 0.2f : 0.0f;

        glyph_font->atlas = std::make_unique<GlyphAtlas>(settings);

        if (!glyph_font->atlas->IsUsable())
        {
          _logger->Error(
            "The font {} cannot be drawn at {} pixels: the font cannot be used", face->path, atlas_size);
        } else if (!is_distance_field)
        {
          // The characters of most text are drawn ahead, so that showing
          // a text for the first time does little work. Everything else
          // is drawn when it is first asked for.
          for (const auto &[first, last] : FontAtlas::DefaultCharacters())
          {
            for (char32_t character = first; character <= last; character++)
            {
              unsigned int glyph = 0;
              if (_rasterizer->GetGlyph(face->font, character, glyph)) { (void) glyph_font->atlas->Find(glyph); }
            }
          }
        }
      }

      if (!glyph_font->atlas->IsUsable() || glyph_font->is_refused) { continue; }

      // handed over now, so that a renderer that takes no texture is known
      // before a text is measured with the font
      if (glyph_font->atlas->GetPageCount() > 0 && GetGlyphTexture(*glyph_font, 0) == No_Texture) { continue; }

      TextFont font;
      font.rasterizer = _rasterizer;
      font.font = face->font;
      font.shaper = face->shaper_font >= 0 ? _shaper : nullptr;
      font.shaper_font = face->shaper_font;
      font.atlas = glyph_font->atlas.get();
      font.pixel_size = static_cast<float>(pixel_size);
      font.scale = static_cast<float>(pixel_size) / static_cast<float>(atlas_size);
      font.places_at_parts = _rasterizer->PlacesAtPartsOfAPixel() && !is_distance_field;

      fonts->fonts.push_back(font);
      fonts->glyphs.push_back(glyph_font.get());
    }

    if (fonts->fonts.empty()) { fonts.reset(); }

    // what failed is kept as well, so that it is not tried again
    return (_text_fonts[key] = std::move(fonts)).get();
  }

  int UiResources::GetGlyphTexture(UiGlyphFont &font, const int page)
  {
    if (font.is_refused || font.atlas == nullptr || page < 0 ||
        static_cast<std::size_t>(page) >= font.atlas->GetPageCount())
    {
      return No_Texture;
    }

    if (font.textures.size() < font.atlas->GetPageCount())
    {
      font.textures.resize(font.atlas->GetPageCount(), No_Texture);
    }

    const GlyphPage &pixels = font.atlas->GetPage(page);
    int &texture = font.textures[page];

    if (texture != No_Texture && !pixels.is_dirty) { return texture; }

    if (texture != No_Texture && !_renderer->UpdateTexture(texture, 0, 0, pixels.width, pixels.height, pixels.pixels))
    {
      // a renderer that cannot write to a texture is given a new one
      _renderer->DestroyTexture(texture);
      texture = No_Texture;
    }

    if (texture == No_Texture)
    {
      texture = _renderer->CreateTexture(pixels.width, pixels.height, pixels.pixels);

      if (texture == No_Texture)
      {
        const int pixel_size = static_cast<int>(font.atlas->GetPixelSize());
        _logger->Error(
          "The renderer took no texture for the font {} at {} pixels", font.path, pixel_size);
        font.is_refused = true;
        return No_Texture;
      }
    }

    font.atlas->MarkClean(page);
    return texture;
  }

  UiImage UiResources::GetImage(const std::string &path)
  {
    if (const auto found = _images.find(path); found != _images.end()) { return found->second; }

    if (_decoder != nullptr) { return LoadBitmap(path, ""); }

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

  UiImage UiResources::LoadBitmap(const std::string &path, const std::string &element)
  {
    if (const auto found = _images.find(path); found != _images.end()) { return found->second; }

    const std::string asked = element.empty() ? "what asks for it" : element;

    UiImage image;
    ImagePixels pixels;

    if (std::vector<unsigned char> file; !_file_system->ReadBytes(path, file))
    {
      _logger->Error("The image {} cannot be read, {} is drawn without it", path, asked);
    } else if (std::string error; !_decoder->Decode(file, pixels, error))
    {
      _logger->Error("The image {} cannot be used, {} is drawn without it: {}", path, asked, error);
    } else
    {
      // with smaller copies, so that an image that is drawn smaller than
      // it is has no rough edges
      TextureOptions2D options;
      options.has_smaller_copies = true;

      image.texture = _renderer->CreateTextureWith(pixels.width, pixels.height, pixels.pixels, options);

      if (image.texture == No_Texture)
      {
        _logger->Error("The renderer took no texture for the image {}, {} is drawn without it", path, asked);
      } else
      {
        image.width = pixels.width;
        image.height = pixels.height;
      }
    }

    // what failed is kept as well, so that it is not tried again
    _images[path] = image;
    return image;
  }

  void UiResources::SetImageDecoder(ImageDecoder *decoder)
  {
    _decoder = decoder;
  }

  void UiResources::SetVectorImageRasterizer(VectorImageRasterizer *rasterizer)
  {
    _vectors = rasterizer;
  }

  void UiResources::SetDocumentFormat(DocumentFormat *format)
  {
    _format = format;
  }

  void UiResources::BeginFrame()
  {
    _frame++;
    _waits_for_surface = false;

    for (const int texture : _released) { _renderer->DestroyTexture(texture); }
    _released.clear();
  }

  std::size_t UiResources::GetVectorPicturesDrawn() const
  {
    return _vector_pictures_drawn;
  }

  void UiResources::WarnOnce(const std::string &about, const std::string &message)
  {
    if (std::ranges::find(_said, about) != _said.end()) { return; }

    _said.push_back(about);
    _logger->Warn("{}", message);
  }

  UiImage UiResources::GetImageFor(
    const std::string &path,
    const float width,
    const float height,
    const float scale,
    const std::string &element)
  {
    if (path.empty()) { return {}; }

    // What a render target was drawn to: a surface of a user interface,
    // or what a camera sees. It may be made after what shows it, so it is
    // looked for whenever it is asked for, and missed once.
    if (const TextureSource source = TextureSource::Of(path); source.kind == TextureSourceKind::Surface)
    {
      const std::string &name = source.name;
      const int target = _renderer->FindRenderTarget(name);

      UiImage image;
      if (target != No_Render_Target) { image.texture = _renderer->GetRenderTargetTexture(target); }

      if (image.texture == No_Texture || !_renderer->GetRenderTargetSize(target, image.width, image.height))
      {
        const std::string asked = element.empty() ? "what asks for it" : element;
        WarnOnce(
          "surface " + name,
          std::format("There is no surface '{}', {} is drawn without it until there is", name, asked));

        // looked for again in the next frame
        _waits_for_surface = true;
        return {};
      }

      return image;
    }

    if (const std::size_t hash = path.find('#'); hash != std::string::npos)
    {
      return GetRegion(path.substr(0, hash), path.substr(hash + 1), element);
    }

    const auto ends_with = [&path](const std::string &end)
    {
      if (path.size() < end.size()) { return false; }

      for (std::size_t i = 0; i < end.size(); i++)
      {
        const char letter = path[path.size() - end.size() + i];
        const char lowered = letter >= 'A' && letter <= 'Z' ? static_cast<char>(letter - 'A' + 'a') : letter;
        if (lowered != end[i]) { return false; }
      }
      return true;
    };

    if (ends_with(".svg"))
    {
      // its own size at the scale of the screen, unless one is asked for
      return GetVectorImage(path, width, height, element);
    }

    (void) scale;
    return _decoder != nullptr ? LoadBitmap(path, element) : GetImage(path);
  }

  bool UiResources::DrawPicture(
    VectorImage &image,
    const std::string &path,
    const int width,
    const int height,
    const std::string &element)
  {
    ImagePixels pixels;
    int texture = No_Texture;

    if (_vectors->Rasterize(image.image, width, height, pixels))
    {
      // with smaller copies, for the frames in which it is drawn at
      // another size than it was made for
      TextureOptions2D options;
      options.has_smaller_copies = true;

      texture = _renderer->CreateTextureWith(pixels.width, pixels.height, pixels.pixels, options);
    }

    if (texture == No_Texture)
    {
      if (!image.is_reported)
      {
        image.is_reported = true;
        const std::string asked = element.empty() ? "what asks for it" : element;
        _logger->Error(
          "The image {} cannot be drawn at {} by {} pixels, {} is drawn with what there is of it",
          path, width, height, asked);
      }
      return false;
    }

    _vector_pictures_drawn++;

    // the picture that was not drawn with for longest makes room
    if (image.pictures.size() >= kMax_Vector_Pictures)
    {
      auto oldest = image.pictures.begin();
      for (auto each = image.pictures.begin(); each != image.pictures.end(); ++each)
      {
        if (each->used_in < oldest->used_in) { oldest = each; }
      }

      _released.push_back(oldest->texture);
      image.pictures.erase(oldest);
    }

    image.pictures.push_back({width, height, texture, _frame});
    return true;
  }

  UiImage UiResources::GetVectorImage(
    const std::string &path,
    const float width,
    const float height,
    const std::string &element)
  {
    const std::string asked = element.empty() ? "what asks for it" : element;

    auto found = _vector_images.find(path);

    if (found == _vector_images.end())
    {
      VectorImage image;

      if (_vectors == nullptr)
      {
        _logger->Error(
          "The image {} is made of shapes, which nothing here draws. {} is drawn without it", path, asked);
      } else if (std::vector<unsigned char> file; !_file_system->ReadBytes(path, file))
      {
        _logger->Error("The image {} cannot be read, {} is drawn without it", path, asked);
      } else if (std::string error; (image.image = _vectors->Load(file, error)) < 0)
      {
        _logger->Error("The image {} cannot be used, {} is drawn without it: {}", path, asked, error);
      } else if (!_vectors->GetSize(image.image, image.natural_width, image.natural_height))
      {
        _vectors->Unload(image.image);
        image.image = -1;
      }

      found = _vector_images.emplace(path, image).first;
    }

    VectorImage &image = found->second;

    UiImage result;
    if (image.image < 0) { return result; }

    result.natural_width = image.natural_width;
    result.natural_height = image.natural_height;

    // what is asked for without a size is asked for to be measured
    const int wanted_width = std::max(1, static_cast<int>(std::lround(width > 0.0f ? width : image.natural_width)));
    const int wanted_height =
      std::max(1, static_cast<int>(std::lround(height > 0.0f ? height : image.natural_height)));

    const auto exact = std::ranges::find_if(image.pictures, [&](const VectorImage::Picture &picture)
    {
      return picture.width == wanted_width && picture.height == wanted_height;
    });

    const VectorImage::Picture *chosen = exact != image.pictures.end() ? &*exact : nullptr;

    if (chosen == nullptr && (width <= 0.0f || height <= 0.0f) && !image.pictures.empty())
    {
      // for its size alone, which every picture knows
      chosen = &image.pictures.back();
    }

    if (chosen == nullptr)
    {
      // The first picture is drawn at once. Another size is drawn once it
      // has been asked for in several frames in a row, which is when what
      // asks has stopped growing.
      bool is_due = image.pictures.empty();

      if (!is_due)
      {
        if (image.wanted_width == wanted_width && image.wanted_height == wanted_height)
        {
          if (image.wanted_in != _frame) { image.wanted_for++; }
        } else
        {
          image.wanted_width = wanted_width;
          image.wanted_height = wanted_height;
          image.wanted_for = 1;
        }

        image.wanted_in = _frame;
        is_due = image.wanted_for >= kSettle_Frames;
      }

      if (is_due && DrawPicture(image, path, wanted_width, wanted_height, element))
      {
        chosen = &image.pictures.back();
        image.wanted_for = 0;
      }
    }

    if (chosen == nullptr && !image.pictures.empty())
    {
      // the picture whose size is nearest, which the renderer scales
      chosen = &image.pictures.front();
      for (const auto &picture : image.pictures)
      {
        if (std::abs(picture.width - wanted_width) < std::abs(chosen->width - wanted_width)) { chosen = &picture; }
      }
    }

    if (chosen == nullptr) { return result; }

    for (auto &picture : image.pictures)
    {
      if (&picture == chosen) { picture.used_in = _frame; }
    }

    result.texture = chosen->texture;
    result.width = chosen->width;
    result.height = chosen->height;
    return result;
  }

  UiResources::Atlas UiResources::ReadAtlas(const std::string &path, const std::string &element)
  {
    Atlas atlas;
    const std::string asked = element.empty() ? "what asks for it" : element;

    if (_format == nullptr)
    {
      _logger->Error("The atlas {} cannot be read, since nothing here reads its format", path);
      return atlas;
    }

    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      _logger->Error("The atlas {} cannot be read, {} is drawn without its image", path, asked);
      return atlas;
    }

    DataValue file;
    if (std::string error; !_format->Read(path, text, file, error))
    {
      _logger->Error("{}", error);
      _logger->Error("The atlas {} has 1 problem and is not used", path);
      return atlas;
    }

    std::vector<std::string> errors;
    const DataReader reader(file, path, "the atlas", errors);

    std::string name;
    std::string image_path;
    reader.Read("atlas", name);

    if (float version = 0.0f; reader.Read("version", version) && version > 1.0f)
    {
      reader.Report(*reader.ReadValue("version"), std::format(
                      "the atlas has version {}, and this engine reads up to version 1", version));
    }

    if (!reader.Read("image", image_path) && !reader.Has("image"))
    {
      reader.Report("the atlas has no 'image', where the virtual path of an image was expected");
    }

    if (reader.Read("scale", atlas.scale) && atlas.scale <= 0.0f)
    {
      reader.Report(*reader.ReadValue("scale"), std::format(
                      "'scale' of the atlas is {}, where a number above 0 was expected", atlas.scale));
      atlas.scale = 1.0f;
    }

    struct Region
    {
      std::string name;
      float x = 0.0f;
      float y = 0.0f;
      float width = 0.0f;
      float height = 0.0f;
      bool has_slice = false;
      LayoutEdges<float> slice;
      std::size_t line = 0;
    };

    std::vector<Region> regions;

    if (const auto *written = reader.ReadValue("regions"); written == nullptr)
    {
      reader.Report("the atlas has no 'regions', where a list of the parts of its image was expected");
    } else if (!written->IsList())
    {
      reader.Report(*written, std::format(
                      "'regions' of the atlas is {}, where a list was expected",
                      DataValue::Describe(written->GetKind())));
    } else
    {
      std::size_t number = 1;
      for (const auto &item : written->GetItems())
      {
        std::string region_name;
        if (const auto *value = item.Find("name"); value != nullptr) { (void) value->GetText(region_name); }

        const std::string where = region_name.empty()
          ? std::format("region {}", number)
          : std::format("region '{}'", region_name);
        number++;

        const DataReader region_reader(item, path, where, errors);

        Region region;
        region.line = item.GetLine();

        const bool has_name = region_reader.Read("name", region.name) && !region.name.empty();
        if (!has_name && !region_reader.Has("name"))
        {
          region_reader.Report(std::format("{} has no 'name', where what it is asked for by was expected", where));
        }

        bool is_placed = true;
        for (const auto &[key, number_of] : {
               std::pair<std::string, float *>{"x", &region.x},
               std::pair<std::string, float *>{"y", &region.y},
               std::pair<std::string, float *>{"width", &region.width},
               std::pair<std::string, float *>{"height", &region.height}
             })
        {
          if (!region_reader.Read(key, *number_of))
          {
            is_placed = false;
            if (!region_reader.Has(key))
            {
              region_reader.Report(std::format(
                "{} has no '{}', where a number of pixels was expected", where, key));
            }
          }
        }

        if (is_placed && (region.x < 0.0f || region.y < 0.0f || region.width <= 0.0f || region.height <= 0.0f))
        {
          region_reader.Report(std::format(
            "{} is at {}, {} with a size of {} by {}, where a place that is not below 0 and a size above 0 "
            "were expected",
            where, region.x, region.y, region.width, region.height));
          is_placed = false;
        }

        if (const auto *slice = region_reader.ReadValue("slice"); slice != nullptr)
        {
          float numbers[4] = {0.0f, 0.0f, 0.0f, 0.0f};
          std::size_t count = 0;
          bool is_read = true;

          if (slice->GetNumber(numbers[0]))
          {
            count = 1;
          } else if (slice->IsList() && !slice->GetItems().empty() && slice->GetItems().size() <= 4)
          {
            count = slice->GetItems().size();
            for (std::size_t i = 0; i < count; i++)
            {
              is_read = is_read && slice->GetItems()[i].GetNumber(numbers[i]) && numbers[i] >= 0.0f;
            }
          } else
          {
            is_read = false;
          }

          if (!is_read || numbers[0] < 0.0f)
          {
            region_reader.Report(*slice, std::format(
                                   "'slice' of {} is {}, where one to four numbers of pixels that are not "
                                   "below 0 were expected: top, right, bottom, left",
                                   where, DataValue::Describe(slice->GetKind())));
          } else
          {
            region.has_slice = true;
            region.slice.top = numbers[0];
            region.slice.right = count > 1 ? numbers[1] : numbers[0];
            region.slice.bottom = count > 2 ? numbers[2] : numbers[0];
            region.slice.left = count > 3 ? numbers[3] : region.slice.right;
          }
        }

        region_reader.Finish();

        if (has_name && is_placed)
        {
          const bool is_known = std::ranges::any_of(regions, [&region](const Region &other)
          {
            return other.name == region.name;
          });

          if (is_known)
          {
            region_reader.Report(std::format(
              "{} shares its name with a region before it. A name says which part is asked for, and is given "
              "once",
              where));
          } else
          {
            regions.push_back(region);
          }
        }
      }
    }

    reader.Finish();

    UiImage image;
    if (errors.empty() && !image_path.empty())
    {
      image = _decoder != nullptr ? LoadBitmap(image_path, "the atlas " + path) : GetImage(image_path);

      if (image.texture == No_Texture)
      {
        errors.push_back(std::format("{}: the image {} of the atlas cannot be used", path, image_path));
      }
    }

    if (errors.empty())
    {
      for (const auto &region : regions)
      {
        if (region.x + region.width > static_cast<float>(image.width) ||
            region.y + region.height > static_cast<float>(image.height))
        {
          errors.push_back(std::format(
            "{}:{}: region '{}' reaches to {}, {}, and the image {} is {} by {}",
            path, region.line, region.name, region.x + region.width, region.y + region.height,
            image_path, image.width, image.height));
        }
      }
    }

    if (!errors.empty())
    {
      for (const auto &error : errors) { _logger->Error("{}", error); }

      const std::size_t count = errors.size();
      const std::string problems = count == 1 ? "problem" : "problems";
      _logger->Error("The atlas {} has {} {} and is not used", path, count, problems);
      return atlas;
    }

    atlas.is_usable = true;
    atlas.image = image;

    for (const auto &region : regions)
    {
      UiImage part = image;
      part.width = static_cast<int>(std::lround(region.width));
      part.height = static_cast<int>(std::lround(region.height));
      part.part_left = region.x / static_cast<float>(image.width);
      part.part_top = region.y / static_cast<float>(image.height);
      part.part_right = (region.x + region.width) / static_cast<float>(image.width);
      part.part_bottom = (region.y + region.height) / static_cast<float>(image.height);
      part.natural_width = region.width / atlas.scale;
      part.natural_height = region.height / atlas.scale;
      part.has_slice = region.has_slice;
      part.slice = region.slice;

      atlas.regions[region.name] = part;
    }

    return atlas;
  }

  UiImage UiResources::GetRegion(const std::string &path, const std::string &name, const std::string &element)
  {
    auto found = _atlases.find(path);
    if (found == _atlases.end()) { found = _atlases.emplace(path, ReadAtlas(path, element)).first; }

    Atlas &atlas = found->second;
    if (!atlas.is_usable) { return {}; }

    if (const auto region = atlas.regions.find(name); region != atlas.regions.end()) { return region->second; }

    if (std::ranges::find(atlas.unknown, name) == atlas.unknown.end())
    {
      atlas.unknown.push_back(name);

      std::string known;
      for (const auto &[each, image] : atlas.regions) { known += (known.empty() ? "" : ", ") + each; }

      const std::string asked = element.empty() ? "what asks for it" : element;
      _logger->Error(
        "The atlas {} has no region '{}', {} is drawn without it. Known are: {}", path, name, asked, known);
    }

    return {};
  }

  int UiResources::GetMaterial(const std::string &shader_path, const std::string &element)
  {
    if (const auto found = _materials.find(shader_path); found != _materials.end()) { return found->second; }

    const int material = _renderer->CreateMaterial(shader_path);

    if (material == No_Material)
    {
      _logger->Error(
        "The shader {} of {} cannot be used, what asks for it is drawn without it", shader_path, element);
    }

    // what failed is kept as well, so that it is not tried again
    _materials[shader_path] = material;
    return material;
  }

  std::vector<MaterialValue2D> UiResources::GetMaterialValues(const UiStyle &style, const UiValues *values)
  {
    std::vector<MaterialValue2D> result;
    result.reserve(style.shader_values.size());

    for (const auto &value : style.shader_values)
    {
      MaterialValue2D each;
      each.name = value.name;
      each.count = static_cast<int>(value.count);
      for (int i = 0; i < 4; i++) { each.numbers[i] = value.numbers[i]; }

      if (!value.bound_to.empty() && values != nullptr)
      {
        // a value that is not set counts as 0, and is reported where the
        // values of a frame are looked at
        const UiValue *found = values->Find(value.bound_to);
        each.numbers[0] = found != nullptr ? static_cast<float>(found->AsNumber(0.0)) : 0.0f;
        each.count = 1;
      }

      result.push_back(each);
    }

    return result;
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

    for (auto &[path, image] : _vector_images)
    {
      for (const auto &picture : image.pictures) { _renderer->DestroyTexture(picture.texture); }
      if (image.image >= 0 && _vectors != nullptr) { _vectors->Unload(image.image); }
    }
    _vector_images.clear();

    for (const int texture : _released) { _renderer->DestroyTexture(texture); }
    _released.clear();

    // the images of the atlases are among the images above
    _atlases.clear();

    for (const auto &[path, material] : _materials)
    {
      if (material != No_Material) { _renderer->DestroyMaterial(material); }
    }
    _materials.clear();
    _said.clear();

    for (const auto &[key, font] : _glyph_fonts)
    {
      for (const int texture : font->textures)
      {
        if (texture != No_Texture) { _renderer->DestroyTexture(texture); }
      }
    }
    _glyph_fonts.clear();
    _text_fonts.clear();
    _fonts_revision++;

    for (auto &face : _faces)
    {
      if (face.font >= 0) { _rasterizer->UnloadFont(face.font); }
      if (face.shaper_font >= 0 && _shaper != nullptr) { _shaper->UnloadFont(face.shaper_font); }
      face.font = -1;
      face.shaper_font = -1;
      face.is_tried = false;
    }

    _unknown_families.clear();
  }
} // neon
