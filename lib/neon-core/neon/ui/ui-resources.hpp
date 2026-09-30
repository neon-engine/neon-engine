#ifndef UI_RESOURCES_HPP
#define UI_RESOURCES_HPP

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/text/font-atlas.hpp>
#include <neon/text/font-rasterizer.hpp>

namespace neon
{
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
    int width = 0;
    int height = 0;
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

  public:
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
  };
} // neon

#endif //UI_RESOURCES_HPP
