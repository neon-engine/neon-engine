#ifndef TEXTURE_SOURCE_HPP
#define TEXTURE_SOURCE_HPP

#include <string>
#include <string_view>

#include "texture-source-kind.hpp"

namespace neon
{
  /// What the path of a texture names, which its start says.
  ///
  /// A texture is named by a path everywhere, in a recipe, a prefab, a
  /// field. Most paths name a file, `assets://textures/wood.png`. Two starts
  /// name something the renderer holds already and no file: `surface://` for
  /// what a render target was drawn to, and `image://` for pixels that were
  /// handed over in memory. They are no schemes of the file system, which
  /// neither reads nor lists them, see docs/file-systems.md.
  ///
  /// This is the one place the two are written down. A renderer asks Of()
  /// what a path names, and whatever makes such a path asks For().
  struct TextureSource
  {
    static constexpr std::string_view surface_scheme = "surface://";
    static constexpr std::string_view image_scheme = "image://";

    TextureSourceKind kind = TextureSourceKind::File;

    /// The name of the surface or of the image, or the path of the file as
    /// it was given.
    std::string name;

    /// What a path names. A surface or an image without a name is taken as
    /// a file, which then is not found.
    [[nodiscard]] static TextureSource Of(const std::string &path)
    {
      if (path.starts_with(surface_scheme) && path.size() > surface_scheme.size())
      {
        return {TextureSourceKind::Surface, path.substr(surface_scheme.size())};
      }
      if (path.starts_with(image_scheme) && path.size() > image_scheme.size())
      {
        return {TextureSourceKind::Image, path.substr(image_scheme.size())};
      }
      return {TextureSourceKind::File, path};
    }

    /// The path that names a surface or an image. A file is named by its
    /// own path.
    [[nodiscard]] static std::string For(const TextureSourceKind kind, const std::string &name)
    {
      switch (kind)
      {
        case TextureSourceKind::Surface: return std::string(surface_scheme) + name;
        case TextureSourceKind::Image: return std::string(image_scheme) + name;
        case TextureSourceKind::File:
        default: return name;
      }
    }
  };
} // neon

#endif //TEXTURE_SOURCE_HPP
