#ifndef VK_TEXTURE_CACHE_HPP
#define VK_TEXTURE_CACHE_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "vk-device.hpp"
#include "vk-texture.hpp"

namespace neon
{
  /// The textures the materials of the render objects read, held once each.
  ///
  /// A texture is held once for every key, however many materials read it:
  /// the first one loads it, the others take a reference, and it is freed
  /// when the last one releases it. The key is the path of the image and
  /// how it is kept, and for an image a model carries inside it, the path
  /// of the model as well, since two models may each call their image `*0`.
  // ReSharper disable once CppInconsistentNaming
  class VK_TextureCache
  {
    /// A texture that is held, and how many materials hold it.
    struct Shared
    {
      VK_Texture texture;
      int count = 0;
    };

    std::map<std::string, Shared> _shared;

    FileSystemContext *_file_system_context = nullptr;
    VK_Device *_device = nullptr;
    std::shared_ptr<Logger> _logger;

    // how many textures were made from a file, and how many times one that
    // was made already was taken instead
    std::size_t _loads = 0;
    std::size_t _shares = 0;

  public:
    void Initialize(FileSystemContext *file_system_context, VK_Device *device, const std::shared_ptr<Logger> &logger);

    /// The key a texture is held under. `model_path` is the model that
    /// carries the image inside it, or empty for an image that is a file
    /// of its own.
    [[nodiscard]] static std::string KeyOf(
      const std::string &path,
      const std::string &model_path,
      const VK_TextureOptions &options);

    /// Hands over the texture of a key, making it when it is not held yet:
    /// from `file`, the bytes of an image a model carries, when there is
    /// one, and from the image at `path` otherwise. Returns false when it
    /// cannot be made, which is logged.
    bool Acquire(
      const std::string &key,
      const std::string &path,
      const std::shared_ptr<const std::vector<unsigned char>> &file,
      const VK_TextureOptions &options,
      VK_Texture &texture);

    /// Gives a texture back. It is freed when nothing holds it any more.
    void Release(const std::string &key);

    /// Frees every texture, whatever still held one.
    void CleanUp();

    /// How many textures are held.
    [[nodiscard]] std::size_t Size() const { return _shared.size(); }

    /// How many textures were made from a file since the cache was made.
    [[nodiscard]] std::size_t Loads() const { return _loads; }

    /// How many times a material took a texture that was made already.
    [[nodiscard]] std::size_t Shares() const { return _shares; }
  };
} // neon

#endif //VK_TEXTURE_CACHE_HPP
