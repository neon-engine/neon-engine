#ifndef TEXTURE_SOURCE_KIND_HPP
#define TEXTURE_SOURCE_KIND_HPP

namespace neon
{
  /// Where a texture comes from, see TextureSource.
  enum class TextureSourceKind
  {
    /// An image file, at a virtual path of the file system.
    File = 0,

    /// What a render target was drawn to, such as a camera's view or a
    /// user interface in the world: `surface://<name>`.
    Surface,

    /// Pixels in memory that were handed to the renderer under a name, see
    /// RenderContext::SetImage: `image://<name>`.
    Image
  };
} // neon

#endif //TEXTURE_SOURCE_KIND_HPP
