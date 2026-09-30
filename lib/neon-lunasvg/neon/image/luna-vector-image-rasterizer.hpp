#ifndef LUNA_VECTOR_IMAGE_RASTERIZER_HPP
#define LUNA_VECTOR_IMAGE_RASTERIZER_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/image/vector-image-rasterizer.hpp>

namespace neon
{
  /// Draws SVG images, with LunaSVG.
  ///
  /// | Of SVG | |
  /// |---|---|
  /// | Shapes, paths, groups, transforms | Drawn |
  /// | Fills, strokes, gradients, patterns, opacity | Drawn |
  /// | Clipping paths, masks, markers, `use` | Drawn |
  /// | Style sheets inside the file | Read |
  /// | Images inside the file, as `data:` | Drawn |
  /// | Images that are files of their own | Refused, with the whole SVG. A library must not open files by itself |
  /// | Text | Not drawn, since no font is handed to the library. Text is turned into paths by the program that writes the SVG |
  /// | Filters, animation, scripts | Not drawn |
  ///
  /// An SVG has to come from where the assets of the game come from, and
  /// not from a player.
  // ReSharper disable once CppInconsistentNaming
  class LUNA_VectorImageRasterizer final : public VectorImageRasterizer
  {
    struct Image;

    // an image is known by its place in here, which is empty once it is
    // unloaded
    std::vector<std::unique_ptr<Image>> _images;

    [[nodiscard]] const Image *Find(int image) const;

  public:
    /// The largest picture that is drawn, along each side.
    static constexpr int kMax_Size = 8192;

    LUNA_VectorImageRasterizer();

    ~LUNA_VectorImageRasterizer();

    LUNA_VectorImageRasterizer(const LUNA_VectorImageRasterizer &) = delete;

    LUNA_VectorImageRasterizer &operator=(const LUNA_VectorImageRasterizer &) = delete;

    int Load(const std::vector<unsigned char> &file, std::string &error) override;

    void Unload(int image) override;

    bool GetSize(int image, float &width, float &height) override;

    bool Rasterize(int image, int width, int height, ImagePixels &pixels) override;

    /// The file an SVG refers to with an `image` that is not part of the
    /// SVG itself, or empty when it refers to none.
    [[nodiscard]] static std::string FindFileOfAnImage(const std::string &svg);
  };
} // neon

#endif //LUNA_VECTOR_IMAGE_RASTERIZER_HPP
