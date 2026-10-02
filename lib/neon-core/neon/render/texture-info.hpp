#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <memory>
#include <string>
#include <vector>

#include "texture-type.hpp"

namespace neon
{
  struct TextureInfo
  {
    /// Virtual path of the image, resolved from the folder of the model
    /// that named it. For an image kept inside the model it is the name the
    /// model gives it, such as `*0`, and `file` holds the image.
    std::string path;
    TextureType texture_type;

    /// The bytes of the image file when the model carries the image itself,
    /// as a GLB does. Empty for an image that is a file of its own. Shared,
    /// so that the meshes of a model copy the name and not the image.
    std::shared_ptr<const std::vector<unsigned char>> file;

    [[nodiscard]] bool IsEmbedded() const { return file != nullptr; }
  };
} // neon

#endif //TEXTURE_HPP
