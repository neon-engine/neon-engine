#ifndef GLYPH_ATLAS_HPP
#define GLYPH_ATLAS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "font-rasterizer.hpp"

namespace neon
{
  /// What is done to the picture of a glyph before it is kept.
  struct GlyphEffect
  {
    enum class Kind : std::uint8_t
    {
      None = 0,

      /// The line around the glyph, `amount` pixels wide.
      Stroke,

      /// The glyph out of focus, as a shadow is. `amount` is the radius
      /// of the blur as CSS counts it, which is twice the deviation.
      Blur
    };

    Kind kind = Kind::None;
    float amount = 0.0f;
  };

  /// Where a glyph is in an atlas, and how it is placed when drawn.
  struct CachedGlyph
  {
    /// The page the picture is on, and where, in pixels. Empty for a glyph
    /// that draws nothing.
    int page = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    /// From the pen to the left edge, and from the baseline up to the top
    /// edge of the picture.
    int left = 0;
    int top = 0;

    float advance = 0.0f;

    /// Whether the picture has colours of its own, as an emoji has. It is
    /// then not drawn in the colour of the text.
    bool has_colors = false;
  };

  /// One image of an atlas. It has a size that does not change, so that
  /// where a glyph lies in it stays true for as long as the atlas lives.
  struct GlyphPage
  {
    int width = 0;
    int height = 0;

    /// Red, green, blue, and alpha for each pixel, row after row from the
    /// top. White with the glyph in the alpha channel, but for glyphs with
    /// colours of their own.
    std::vector<unsigned char> pixels;

    /// Whether glyphs were added since the page was last handed to a
    /// renderer.
    bool is_dirty = false;
  };

  /// The glyphs of one font at one size, each drawn when it is first asked
  /// for.
  ///
  /// Nothing says beforehand which glyphs a text needs: a game may show
  /// Latin, Arabic, or Chinese, and shaping hands over glyphs that no
  /// character stands for. So the atlas starts empty and grows. When a page
  /// is full, another one is added.
  ///
  /// A glyph is kept in up to four pictures, each moved by a quarter of a
  /// pixel more than the one before. A text picks the one that is nearest
  /// to where its pen is, which spaces small text evenly.
  class GlyphAtlas
  {
  public:
    /// Pictures of a glyph from side to side of a pixel.
    static constexpr int kVariants = 4;

    /// The largest page, along each side.
    static constexpr int kMax_Page_Size = 2048;

    struct Settings
    {
      FontRasterizer *rasterizer = nullptr;
      int font = -1;
      float pixel_size = 0.0f;
      GlyphRendering rendering = GlyphRendering::Bitmap;
      float embolden = 0.0f;
      float slant = 0.0f;

      /// The size of a page along each side. 0 stands for one that suits
      /// the size of the font.
      int page_size = 0;
    };

  private:
    struct Key
    {
      unsigned int glyph = 0;
      std::uint8_t variant = 0;
      std::uint8_t effect = 0;
      std::uint16_t amount = 0;

      bool operator==(const Key &other) const = default;
    };

    struct KeyHash
    {
      std::size_t operator()(const Key &key) const
      {
        const std::uint64_t packed =
          (static_cast<std::uint64_t>(key.glyph) << 32) |
          (static_cast<std::uint64_t>(key.variant) << 24) |
          (static_cast<std::uint64_t>(key.effect) << 16) |
          key.amount;
        return std::hash<std::uint64_t>{}(packed);
      }
    };

    /// A row of glyphs of about the same height.
    struct Shelf
    {
      int page = 0;
      int y = 0;
      int height = 0;
      int used = 0;
    };

    Settings _settings;
    FontMetrics _metrics;
    bool _is_usable = false;
    int _page_size = 0;
    float _distance_range = 0.0f;

    std::vector<GlyphPage> _pages;
    std::vector<Shelf> _shelves;

    // the next free row of the last page
    int _next_row = 0;

    // A glyph that could not be drawn is kept as well, as nullptr, so that
    // it is not tried again in every frame. Pointers to the others stay
    // true while the atlas lives.
    std::unordered_map<Key, std::unique_ptr<CachedGlyph>, KeyHash> _glyphs;
    std::size_t _failed = 0;

    bool Place(int width, int height, CachedGlyph &glyph);

    void AddPage();

  public:
    explicit GlyphAtlas(const Settings &settings);

    /// Whether the font can be drawn at the size.
    [[nodiscard]] bool IsUsable() const;

    /// The glyph with that number in the font, drawn if it was not yet.
    /// `variant` says by how many quarters of a pixel it is moved to the
    /// right. nullptr for a glyph that cannot be drawn, or that is larger
    /// than a page.
    [[nodiscard]] const CachedGlyph *Find(unsigned int glyph, int variant = 0, const GlyphEffect &effect = {});

    [[nodiscard]] const FontMetrics &GetMetrics() const;

    [[nodiscard]] float GetPixelSize() const;

    [[nodiscard]] bool IsDistanceField() const;

    /// Pixels of a page from the outline of a glyph to where its distances
    /// end. 0 for an atlas of bitmaps, and until the first glyph is drawn.
    [[nodiscard]] float GetDistanceRange() const;

    [[nodiscard]] std::size_t GetPageCount() const;

    [[nodiscard]] const GlyphPage &GetPage(std::size_t page) const;

    /// Says that a page was handed to the renderer as it is now.
    void MarkClean(std::size_t page);

    /// Glyphs that are kept, and glyphs that could not be drawn.
    [[nodiscard]] std::size_t GetGlyphCount() const;

    [[nodiscard]] std::size_t GetFailedCount() const;
  };

  /// Puts a picture out of focus. `coverage` holds a byte for each pixel
  /// and is made larger on every side by what is returned, so that nothing
  /// of the blur is cut off.
  int BlurCoverage(std::vector<unsigned char> &coverage, int &width, int &height, float radius);
} // neon

#endif //GLYPH_ATLAS_HPP
