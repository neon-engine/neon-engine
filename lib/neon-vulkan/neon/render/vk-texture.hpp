#ifndef VK_TEXTURE_HPP
#define VK_TEXTURE_HPP

#include <string>
#include <vector>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/image/image-pixels.hpp>

#include "vk-device.hpp"

namespace neon
{
  /// How a texture is kept and read. What is left as it is suits the
  /// surface of a model.
  // ReSharper disable once CppInconsistentNaming
  struct VK_TextureOptions
  {
    /// Smaller copies for when the image is seen from afar.
    bool mip_levels = true;

    /// Whether the image starts again past its edge. Otherwise its edge is
    /// drawn on.
    bool repeat = true;

    /// Multiplies alpha into the colours before the image is kept, which
    /// is what blending by ONE and ONE_MINUS_SRC_ALPHA expects. A pixel
    /// that is see-through then adds no colour of its own to its
    /// neighbours when the image is scaled.
    bool premultiply_alpha = false;

    /// Whether the image holds colours, which an image file keeps in sRGB.
    /// The graphics card turns them into linear light as they are read,
    /// and makes its smaller copies in linear light, which is what lighting
    /// works in. An image that holds numbers instead, such as how shiny
    /// each part of a surface is, is read as it is. So is an image of a
    /// user interface, which is drawn in sRGB as CSS draws it.
    bool is_color = true;
  };

  // ReSharper disable once CppInconsistentNaming
  class VK_Texture
  {
    std::string _texture_path;
    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;
    bool _initialized = false;

    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkImageView _view = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;
    uint32_t _width = 0;
    uint32_t _height = 0;
    uint32_t _mip_levels = 1;

    // whether the image belongs to something else, which releases it
    bool _is_borrowed = false;

    bool Upload(
      const unsigned char *pixels,
      uint32_t width,
      uint32_t height,
      const VK_TextureOptions &options = {});

  public:
    VK_Texture() = default;

    VK_Texture(
      const std::string &texture_path,
      FileSystemContext *file_system_context,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    /// Loads the image the texture was created with.
    bool Initialize();

    bool Initialize(const VK_TextureOptions &options);

    /// Makes the texture from pixels in memory: red, green, blue, and alpha
    /// for each, row after row from the top.
    bool InitializeWithPixels(
      const unsigned char *pixels,
      uint32_t width,
      uint32_t height,
      const VK_TextureOptions &options);

    /// Makes the texture a single colour. Used where a material names no
    /// texture, so that shaders always have something to read.
    bool InitializeWithColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);

    void CleanUp();

    /// Makes the texture from an image and its smaller copies, the image
    /// first, as MakeSmallerCopies() hands them over. Nothing is done to
    /// the pixels on the way.
    bool InitializeWithLevels(const std::vector<ImagePixels> &levels, const VK_TextureOptions &options);

    /// Replaces a part of the texture. `pixels` holds the part alone. The
    /// texture has to be one without smaller copies.
    bool Update(
      const unsigned char *pixels,
      uint32_t x,
      uint32_t y,
      uint32_t width,
      uint32_t height,
      bool premultiply_alpha);

    /// A texture that reads an image it does not own, such as what a
    /// render target was drawn to. CleanUp() leaves the image alone.
    [[nodiscard]] static VK_Texture Borrowed(VkImageView view, VkSampler sampler, uint32_t width, uint32_t height);

    /// The format a texture is kept in: sRGB for colours, so that they are
    /// read as linear light, and plain bytes for anything else.
    [[nodiscard]] static VkFormat FormatFor(bool is_color);

    /// The two formats: sRGB for colours, which the graphics card turns
    /// into linear light as they are read, and plain bytes for anything
    /// else.
    static constexpr VkFormat kColor_Format = VK_FORMAT_R8G8B8A8_SRGB;
    static constexpr VkFormat kData_Format = VK_FORMAT_R8G8B8A8_UNORM;

    [[nodiscard]] VkImageView View() const { return _view; }
    [[nodiscard]] VkSampler Sampler() const { return _sampler; }
    [[nodiscard]] uint32_t Width() const { return _width; }
    [[nodiscard]] uint32_t Height() const { return _height; }
  };
} // neon

#endif //VK_TEXTURE_HPP
