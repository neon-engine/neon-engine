#ifndef MODEL_GEOMETRY_HPP
#define MODEL_GEOMETRY_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

namespace neon
{
  /// The points and triangles of a model, for the physics to make a shape
  /// from. Nothing of it is for drawing.
  struct ModelGeometry
  {
    std::vector<glm::vec3> points;

    /// Three for every triangle, each the number of a point.
    std::vector<std::uint32_t> triangles;
  };

  /// Reads the geometry of a model through the file system. `path` is a
  /// virtual path, such as `assets://models/bunny.obj`.
  ///
  /// The points are moved and sized the way the renderer does it for what it
  /// draws: the middle of the model lies at the origin, and its longest side
  /// has length 1. So a shape that is made from them lies where the model is
  /// drawn.
  ///
  /// Returns false when the model cannot be read or holds no triangle, which
  /// is logged.
  bool LoadModelGeometry(
    const std::string &path,
    FileSystemContext *file_system_context,
    const std::shared_ptr<Logger> &logger,
    ModelGeometry &geometry);
} // neon

#endif //MODEL_GEOMETRY_HPP
