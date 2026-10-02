#include "glyph-atlas.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  // Helpers of GlyphEffect, for this file alone.
  namespace
  {
    // kept free around every picture
    constexpr int spacing = 1;

    // Glyphs share a row when their heights round to the same step, so
    // that little of a row is left empty above the lower ones.
    constexpr int height_step = 4;

    int PageSizeFor(const float pixel_size, const GlyphRendering rendering)
    {
      if (rendering == GlyphRendering::DistanceField) { return 1024; }
      if (pixel_size <= 24.0f) { return 512; }
      if (pixel_size <= 64.0f) { return 1024; }
      return GlyphAtlas::kMax_Page_Size;
    }

    /// One pass of a blur that weighs every pixel within `reach` the same,
    /// along the rows and then along the columns.
    void BoxBlur(std::vector<float> &values, std::vector<float> &scratch, const int width, const int height, const int reach)
    {
      if (reach <= 0) { return; }

      const float share = 1.0f / static_cast<float>(2 * reach + 1);

      for (int row = 0; row < height; row++)
      {
        const float *from = values.data() + static_cast<std::size_t>(row) * width;
        float *to = scratch.data() + static_cast<std::size_t>(row) * width;

        float sum = 0.0f;
        for (int column = -reach; column <= reach; column++)
        {
          if (column >= 0 && column < width) { sum += from[column]; }
        }

        for (int column = 0; column < width; column++)
        {
          to[column] = sum * share;

          const int leaves = column - reach;
          const int enters = column + reach + 1;
          if (leaves >= 0) { sum -= from[leaves]; }
          if (enters < width) { sum += from[enters]; }
        }
      }

      for (int column = 0; column < width; column++)
      {
        float sum = 0.0f;
        for (int row = -reach; row <= reach; row++)
        {
          if (row >= 0 && row < height) { sum += scratch[static_cast<std::size_t>(row) * width + column]; }
        }

        for (int row = 0; row < height; row++)
        {
          values[static_cast<std::size_t>(row) * width + column] = sum * share;

          const int leaves = row - reach;
          const int enters = row + reach + 1;
          if (leaves >= 0) { sum -= scratch[static_cast<std::size_t>(leaves) * width + column]; }
          if (enters < height) { sum += scratch[static_cast<std::size_t>(enters) * width + column]; }
        }
      }
    }
  }

  int BlurCoverage(std::vector<unsigned char> &coverage, int &width, int &height, const float radius)
  {
    if (radius <= 0.0f || width <= 0 || height <= 0) { return 0; }

    // The radius of CSS is twice the deviation of the bell curve. Three
    // passes of a box come close to the curve, each reaching as far as
    // gives the same deviation: reach * (reach + 1) = deviation squared.
    const float deviation = radius / 2.0f;
    const int reach = std::max(1, static_cast<int>(std::round((std::sqrt(4.0f * deviation * deviation + 1.0f) - 1.0f) / 2.0f)));
    const int margin = 3 * reach;

    const int wide = width + 2 * margin;
    const int high = height + 2 * margin;

    std::vector<float> values(static_cast<std::size_t>(wide) * high, 0.0f);
    for (int row = 0; row < height; row++)
    {
      for (int column = 0; column < width; column++)
      {
        values[static_cast<std::size_t>(row + margin) * wide + column + margin] =
          static_cast<float>(coverage[static_cast<std::size_t>(row) * width + column]);
      }
    }

    std::vector<float> scratch(values.size(), 0.0f);
    for (int pass = 0; pass < 3; pass++) { BoxBlur(values, scratch, wide, high, reach); }

    coverage.resize(values.size());
    for (std::size_t i = 0; i < values.size(); i++)
    {
      coverage[i] = static_cast<unsigned char>(std::clamp(std::lround(values[i]), 0l, 255l));
    }

    width = wide;
    height = high;
    return margin;
  }

  GlyphAtlas::GlyphAtlas(const Settings &settings)
  {
    _settings = settings;

    _page_size = settings.page_size > 0
      ? std::min(settings.page_size, kMax_Page_Size)
      : PageSizeFor(settings.pixel_size, settings.rendering);

    _is_usable = settings.rasterizer != nullptr && settings.pixel_size > 0.0f &&
                 settings.rasterizer->GetMetrics(settings.font, settings.pixel_size, _metrics);
  }

  bool GlyphAtlas::IsUsable() const
  {
    return _is_usable;
  }

  void GlyphAtlas::AddPage()
  {
    GlyphPage page;
    page.width = _page_size;
    page.height = _page_size;

    // white everywhere, so that the colour next to a glyph is the colour
    // of the glyph when the image is scaled
    page.pixels.assign(static_cast<std::size_t>(_page_size) * _page_size * 4, 255);
    for (std::size_t i = 3; i < page.pixels.size(); i += 4) { page.pixels[i] = 0; }

    _pages.push_back(std::move(page));
    _next_row = spacing;
  }

  bool GlyphAtlas::Place(const int width, const int height, CachedGlyph &glyph)
  {
    if (width + 2 * spacing > _page_size || height + 2 * spacing > _page_size) { return false; }

    const int row_height = (height + height_step - 1) / height_step * height_step;

    // a row of that height which has room
    for (auto &shelf : _shelves)
    {
      if (shelf.height != row_height || shelf.used + width + spacing > _page_size) { continue; }

      glyph.page = shelf.page;
      glyph.x = shelf.used;
      glyph.y = shelf.y;
      shelf.used += width + spacing;
      return true;
    }

    if (_pages.empty() || _next_row + row_height + spacing > _page_size)
    {
      AddPage();
    }

    Shelf shelf;
    shelf.page = static_cast<int>(_pages.size()) - 1;
    shelf.y = _next_row;
    shelf.height = row_height;
    shelf.used = spacing + width + spacing;
    _shelves.push_back(shelf);

    _next_row += row_height + spacing;

    glyph.page = shelf.page;
    glyph.x = spacing;
    glyph.y = shelf.y;
    return true;
  }

  const CachedGlyph *GlyphAtlas::Find(const unsigned int glyph, const int variant, const GlyphEffect &effect)
  {
    if (!_is_usable) { return nullptr; }

    Key key;
    key.glyph = glyph;
    key.variant = static_cast<std::uint8_t>(std::clamp(variant, 0, kVariants - 1));
    key.effect = static_cast<std::uint8_t>(effect.amount > 0.0f ? effect.kind : GlyphEffect::Kind::None);

    // to a quarter of a pixel, so that widths that are all but the same
    // share their pictures
    key.amount = key.effect == 0
      ? static_cast<std::uint16_t>(0)
      : static_cast<std::uint16_t>(std::clamp(std::lround(effect.amount * 4.0f), 1l, 65535l));

    if (const auto found = _glyphs.find(key); found != _glyphs.end()) { return found->second.get(); }

    const auto kind = static_cast<GlyphEffect::Kind>(key.effect);
    const float amount = static_cast<float>(key.amount) / 4.0f;

    GlyphOptions options;
    options.offset_x = static_cast<float>(key.variant) / static_cast<float>(kVariants);
    options.rendering = _settings.rendering;
    options.embolden = _settings.embolden;
    options.slant = _settings.slant;
    if (kind == GlyphEffect::Kind::Stroke) { options.stroke = amount; }

    GlyphBitmap bitmap;
    if (!_settings.rasterizer->RasterizeGlyph(_settings.font, _settings.pixel_size, glyph, options, bitmap))
    {
      _failed++;
      return (_glyphs[key] = nullptr).get();
    }

    if (kind == GlyphEffect::Kind::Blur && !bitmap.coverage.empty() && !bitmap.is_distance_field)
    {
      const int margin = BlurCoverage(bitmap.coverage, bitmap.width, bitmap.height, amount);
      bitmap.left -= margin;
      bitmap.top += margin;

      // a shadow has the colour it is given
      bitmap.colors.clear();
    }

    if (bitmap.is_distance_field) { _distance_range = bitmap.distance_range; }

    auto cached = std::make_unique<CachedGlyph>();
    cached->left = bitmap.left;
    cached->top = bitmap.top;
    cached->advance = bitmap.advance;

    if (!bitmap.coverage.empty() && bitmap.width > 0 && bitmap.height > 0)
    {
      if (!Place(bitmap.width, bitmap.height, *cached))
      {
        _failed++;
        return (_glyphs[key] = nullptr).get();
      }

      cached->width = bitmap.width;
      cached->height = bitmap.height;
      cached->has_colors = !bitmap.colors.empty();

      GlyphPage &page = _pages[cached->page];
      page.is_dirty = true;

      for (int row = 0; row < bitmap.height; row++)
      {
        for (int column = 0; column < bitmap.width; column++)
        {
          const std::size_t from = static_cast<std::size_t>(row) * bitmap.width + column;
          const std::size_t to =
            (static_cast<std::size_t>(cached->y + row) * page.width +
             static_cast<std::size_t>(cached->x + column)) * 4;

          if (cached->has_colors)
          {
            for (std::size_t channel = 0; channel < 4; channel++)
            {
              page.pixels[to + channel] = bitmap.colors[from * 4 + channel];
            }
          } else
          {
            page.pixels[to + 3] = bitmap.coverage[from];
          }
        }
      }
    }

    return (_glyphs[key] = std::move(cached)).get();
  }

  const FontMetrics &GlyphAtlas::GetMetrics() const
  {
    return _metrics;
  }

  float GlyphAtlas::GetPixelSize() const
  {
    return _settings.pixel_size;
  }

  bool GlyphAtlas::IsDistanceField() const
  {
    return _settings.rendering == GlyphRendering::DistanceField;
  }

  float GlyphAtlas::GetDistanceRange() const
  {
    return _distance_range;
  }

  std::size_t GlyphAtlas::GetPageCount() const
  {
    return _pages.size();
  }

  const GlyphPage &GlyphAtlas::GetPage(const std::size_t page) const
  {
    return _pages[page];
  }

  void GlyphAtlas::MarkClean(const std::size_t page)
  {
    if (page < _pages.size()) { _pages[page].is_dirty = false; }
  }

  std::size_t GlyphAtlas::GetGlyphCount() const
  {
    return _glyphs.size() - _failed;
  }

  std::size_t GlyphAtlas::GetFailedCount() const
  {
    return _failed;
  }
} // neon
