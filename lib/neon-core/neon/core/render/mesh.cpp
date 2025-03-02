#include "mesh.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace core {
  const std::vector<Texture> & Mesh::GetTextures() const
  {
    return _textures;
  }

  glm::mat4 Mesh::GetModelMatrix() const
  {
    return _model_matrix;
  }

  void Mesh::GenerateNormalizationMatrix()
  {
    _logger->Debug("Generating normalized matrix using the loaded vertices");
    auto min_x = _vertices[0].position.x;
    auto min_y = _vertices[0].position.y;
    auto min_z = _vertices[0].position.z;

    auto max_x = _vertices[0].position.x;
    auto max_y = _vertices[0].position.y;
    auto max_z = _vertices[0].position.z;

    for (int i = 3; i < _vertices.size(); i += 3) {
      const auto x = _vertices[i].position.x;
      const auto y = _vertices[i + 1].position.y;
      const auto z = _vertices[i + 2].position.z;

      if (x < min_x) {min_x = x;}
      if (y < min_y) {min_y = y;}
      if (z < min_z) {min_z = z;}

      if (x > max_x) {max_x = x;}
      if (y > max_y) {max_y = y;}
      if (z > max_z) {max_z = z;}
    }

    const auto x_center = (min_x + max_x) / 2.0f;
    const auto y_center = (min_y + max_y) / 2.0f;
    const auto z_center = (min_z + max_z) / 2.0f;

    const auto range_x = max_x - min_x;
    const auto range_y = max_y - min_y;
    const auto range_z = max_z - min_z;

    const auto max_range = std::max(std::max(range_x, range_y), range_z);

    const auto translation = translate(glm::mat4(1.0f), {-x_center, -y_center, -z_center});
    _model_matrix =
      scale(glm::mat4(1.0f), {1.0f / max_range, 1.0f / max_range, 1.0f / max_range}) * translation;
  }
} // core