#ifndef UI_RESOURCES_HPP
#define UI_RESOURCES_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/image/image-decoder.hpp>
#include <neon/image/vector-image-rasterizer.hpp>
#include <neon/layout/layout-style.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/text/font-atlas.hpp>
#include <neon/text/font-rasterizer.hpp>
#include <neon/text/glyph-atlas.hpp>
#include <neon/text/shaped-text.hpp>
#include <neon/text/text-shaper.hpp>

namespace neon
{
  struct UiStyle;
  class UiValues;

  /// A font at one size, ready to be drawn with.
  struct UiFont
  {
    FontAtlas atlas;
    int texture = No_Texture;
  };

  /// An image, ready to be drawn with.
  struct UiImage
  {
    int texture = No_Texture;

    /// The size of the image in its own pixels. Of a part of an atlas, the
    /// size of the part.
    int width = 0;
    int height = 0;

    /// Where in the texture the image is, in parts of the size of the
    /// texture. The whole of it, but for a part of an atlas.
    float part_left = 0.0f;
    float part_top = 0.0f;
    float part_right = 1.0f;
    float part_bottom = 1.0f;

    /// The size the image has in a user interface, in units of the file.
    /// 0 stands for its size in pixels.
    float natural_width = 0.0f;
    float natural_height = 0.0f;

    /// How far the corners reach into the image when it is drawn in nine
    /// parts, as its atlas says. In pixels of the image.
    bool has_slice = false;
    LayoutEdges<float> slice{};

    [[nodiscard]] float NaturalWidth() const
    {
      return natural_width > 0.0f ? natural_width : static_cast<float>(width);
    }

    [[nodiscard]] float NaturalHeight() const
    {
      return natural_height > 0.0f ? natural_height : static_cast<float>(height);
    }
  };

  /// What is said about a font next to its family, its weight, and its
  /// file.
  struct UiFaceOptions
  {
    /// Whether it is the italic of its family.
    bool is_italic = false;

    /// Whether its glyphs are kept as bitmaps at the size they are drawn
    /// at, or as distances at one size for every size.
    GlyphRendering rendering = GlyphRendering::Bitmap;
  };

  /// The glyphs of a font at one size, and the textures they are in.
  struct UiGlyphFont
  {
    std::unique_ptr<GlyphAtlas> atlas;

    /// One for each page of the atlas, No_Texture for a page that was not
    /// handed to the renderer yet.
    std::vector<int> textures;

    /// The file of the font, for messages.
    std::string path;

    /// Whether the renderer refused a page. Nothing is drawn with the font
    /// then, and nothing is tried again.
    bool is_refused = false;
  };

  /// The fonts a text is drawn with, in the order they are tried in for
  /// every character.
  struct UiTextFonts
  {
    std::vector<TextFont> fonts;

    /// What holds the textures of each of `fonts`.
    std::vector<UiGlyphFont *> glyphs;
  };

  /// The fonts and images of a user interface. Each is loaded when it is
  /// first asked for, and once: what could not be loaded is remembered as
  /// that, reported once, and not tried again in every frame.
  class UiResources
  {
    struct Face
    {
      std::string family;
      int weight = 400;
      std::string path;

      // what the rasterizer knows the font as, once it was asked for
      bool is_tried = false;
      int font = -1;

      UiFaceOptions options;

      // what the shaper knows the font as, or -1
      int shaper_font = -1;
    };

    /// What tells two sets of glyphs of one face apart.
    struct GlyphFontKey
    {
      std::size_t face = 0;
      int pixel_size = 0;
      int embolden = 0;
      int slant = 0;

      auto operator<=>(const GlyphFontKey &other) const = default;
    };

    struct TextFontsKey
    {
      std::string families;
      int weight = 400;
      bool is_italic = false;
      int pixel_size = 0;

      auto operator<=>(const TextFontsKey &other) const = default;
    };

    Render2DContext *_renderer;
    FontRasterizer *_rasterizer;
    FileSystemContext *_file_system;
    std::shared_ptr<Logger> _logger;

    std::vector<Face> _faces;

    // by face and size in pixels. Empty for what could not be made
    std::map<std::pair<std::size_t, int>, std::unique_ptr<UiFont>> _fonts;

    // by path. Without a texture for what could not be loaded
    std::map<std::string, UiImage> _images;

    // families that were asked for and are not known, each reported once
    std::vector<std::string> _unknown_families;

    [[nodiscard]] Face *FindFace(const std::string &family, int weight);

    TextShaper *_shaper = nullptr;

    // goes up whenever the fonts a text could be drawn with have changed
    std::uint64_t _fonts_revision = 0;

    std::map<GlyphFontKey, std::unique_ptr<UiGlyphFont>> _glyph_fonts;

    // Empty for fonts of which none can be used
    std::map<TextFontsKey, std::unique_ptr<UiTextFonts>> _text_fonts;

    [[nodiscard]] Face *FindFace(const std::string &family, int weight, bool is_italic);

    /// An image that is made of shapes, and the pictures that were drawn
    /// of it.
    struct VectorImage
    {
      /// One size it was drawn at.
      struct Picture
      {
        int width = 0;
        int height = 0;
        int texture = No_Texture;
        std::uint64_t used_in = 0;
      };

      // what the rasterizer knows it as, or -1 for one that cannot be used
      int image = -1;
      float natural_width = 0.0f;
      float natural_height = 0.0f;

      std::vector<Picture> pictures;

      // the size that is asked for and was not drawn yet, and for how many
      // frames it has been asked for
      int wanted_width = 0;
      int wanted_height = 0;
      int wanted_for = 0;
      std::uint64_t wanted_in = 0;

      // whether it was said that a picture could not be drawn
      bool is_reported = false;
    };

    /// The parts of an image by their names.
    struct Atlas
    {
      bool is_usable = false;
      UiImage image;
      float scale = 1.0f;
      std::map<std::string, UiImage> regions;

      // names that were asked for and are not there, each said once
      std::vector<std::string> unknown;
    };

    ImageDecoder *_decoder = nullptr;
    VectorImageRasterizer *_vectors = nullptr;
    DocumentFormat *_format = nullptr;

    std::map<std::string, VectorImage> _vector_images;
    std::map<std::string, Atlas> _atlases;

    // by the path of the shader. No_Material for what cannot be used
    std::map<std::string, int> _materials;

    // what is said once, by what it is about
    std::vector<std::string> _said;

    // textures that are no longer drawn with, and are released when no
    // frame refers to them
    std::vector<int> _released;

    std::uint64_t _frame = 1;
    std::size_t _vector_pictures_drawn = 0;

    // whether an image of a surface was asked for in this frame that
    // there is no surface for yet
    bool _waits_for_surface = false;

    [[nodiscard]] UiImage GetVectorImage(
      const std::string &path,
      float width,
      float height,
      const std::string &element);

    [[nodiscard]] UiImage GetRegion(const std::string &path, const std::string &name, const std::string &element);

    [[nodiscard]] Atlas ReadAtlas(const std::string &path, const std::string &element);

    [[nodiscard]] UiImage LoadBitmap(const std::string &path, const std::string &element);

    bool DrawPicture(VectorImage &image, const std::string &path, int width, int height, const std::string &element);

    /// Reads the file of a face when that was not tried yet. Returns
    /// whether the face can be drawn with.
    bool Load(Face &face);

    void ReportUnknown(const std::string &family);

  public:
    /// The size glyphs are kept at that are kept as distances.
    static constexpr int kDistance_Field_Size = 48;

    /// Sizes an image of shapes is kept at. When another is asked for, the
    /// one that was not drawn with for longest makes room.
    static constexpr std::size_t kMax_Vector_Pictures = 4;

    /// Frames in a row a size has to be asked for before an image of
    /// shapes is drawn at it, so that one that is growing is not drawn
    /// again in every frame.
    static constexpr int kSettle_Frames = 3;
    /// Gives up the fonts at the sizes they were drawn at, with their
    /// textures. The files of the fonts stay loaded. Called when the scale
    /// of the user interface changed, since text is then drawn at other
    /// sizes and the old ones would be kept for nothing. Returns how many
    /// textures were destroyed.
    std::size_t ReleaseFonts();

    /// How many fonts are kept at a size, which is how many textures they
    /// hold.
    [[nodiscard]] std::size_t GetFontCount() const;

    /// Whether something asked for in this frame is not there yet and may
    /// be in the next: an image of shapes at a size it is not drawn at,
    /// which is drawn once the size has been asked for in a few frames in
    /// a row, or an image of a surface that does not exist yet.
    [[nodiscard]] bool IsSettling() const;

    UiResources(
      Render2DContext *renderer,
      FontRasterizer *rasterizer,
      FileSystemContext *file_system,
      const std::shared_ptr<Logger> &logger);

    /// Says where the font of a family and a weight is, as `@font-face` of
    /// CSS does. The file is read when the font is first drawn with. A
    /// face that is known already is replaced, unless it was drawn with.
    void AddFace(const std::string &family, int weight, const std::string &path);

    [[nodiscard]] bool HasFamily(const std::string &family) const;

    /// The font of a family at a size in pixels. Of the weights the family
    /// has, the nearest is taken. nullptr when there is no such font.
    [[nodiscard]] const UiFont *GetFont(const std::string &family, int weight, int pixel_size);

    /// The image at a virtual path. It has no texture when it could not be
    /// loaded.
    [[nodiscard]] UiImage GetImage(const std::string &path);

    /// Destroys the textures and unloads the fonts.
    void CleanUp();

    /// What shapes the text from now on, or nullptr for text that is not
    /// shaped. Fonts that were read before are not handed to it.
    void SetTextShaper(TextShaper *shaper);

    /// As AddFace() above, for a face that is an italic or is kept as
    /// distances.
    void AddFace(const std::string &family, int weight, const std::string &path, const UiFaceOptions &options);

    /// The fonts of a list of families as CSS writes it, with commas
    /// between them, at a size in pixels. A family that is not known is
    /// reported once and left out. Of the faces a family has, the nearest
    /// in style and weight is taken, and a bold or an italic that the
    /// family does not have is made from what it has. nullptr when no
    /// font of the list can be used.
    [[nodiscard]] const UiTextFonts *GetTextFonts(
      const std::string &families,
      int weight,
      bool is_italic,
      int pixel_size);

    /// The texture of a page of glyphs. Glyphs that were drawn since it
    /// was last asked for are handed to the renderer first. No_Texture
    /// when the renderer does not take it.
    [[nodiscard]] int GetGlyphTexture(UiGlyphFont &font, int page);

    /// Goes up whenever the fonts a text could be drawn with have
    /// changed, so that what was made with them is made again.
    [[nodiscard]] std::uint64_t GetFontsRevision() const;

    /// What reads image files. Without one, the renderer is asked to load
    /// an image, which leaves it without smaller copies.
    void SetImageDecoder(ImageDecoder *decoder);

    /// What draws images that are made of shapes, such as SVG. Without
    /// one, such an image cannot be used.
    void SetVectorImageRasterizer(VectorImageRasterizer *rasterizer);

    /// What reads the descriptions of atlases.
    void SetDocumentFormat(DocumentFormat *format);

    /// Says that a frame starts. What is no longer drawn with is released
    /// here, where no frame refers to it.
    void BeginFrame();

    /// An image by what a file calls it:
    ///
    ///     assets://ui/heart.png             an image file
    ///     assets://ui/shield.svg            an image of shapes
    ///     assets://ui/icons.atlas.yml#coin  a part of an atlas
    ///     surface://minimap                 what a camera sees, or a surface
    ///
    /// `width` and `height` say how large it is drawn, in pixels, which
    /// an image of shapes is drawn at. 0 stands for its own size times
    /// `scale`. `element` is who asks, for the message when the image
    /// cannot be used.
    [[nodiscard]] UiImage GetImageFor(
      const std::string &path,
      float width,
      float height,
      float scale,
      const std::string &element = "");

    /// How often an image of shapes was drawn, which a test looks at.
    [[nodiscard]] std::size_t GetVectorPicturesDrawn() const;

    /// The shader an element is drawn with, or No_Material when it cannot
    /// be used, which is said once with the element that asked first.
    [[nodiscard]] int GetMaterial(const std::string &shader_path, const std::string &element);

    /// The values of `shader_values` as they are now. A value that
    /// follows one of the game has what that holds.
    [[nodiscard]] static std::vector<MaterialValue2D> GetMaterialValues(
      const UiStyle &style,
      const UiValues *values);

    /// Logs a warning unless one with the same `about` was logged before.
    void WarnOnce(const std::string &about, const std::string &message);
  };
} // neon

#endif //UI_RESOURCES_HPP
