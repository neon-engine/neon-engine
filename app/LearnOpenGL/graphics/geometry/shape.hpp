#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <vector>

class IShape {
 private:
  std::vector<float> _vertices;
  std::vector<int> _indices;
  std::vector<float> _texture_coords;
 public:
  virtual void Draw() const = 0;
  virtual void SetVertices(const std::vector<float> &vertices) = 0;
  virtual void SetIndices(const std::vector<int> &indices) = 0;
  virtual void SetTextureCoords(const std::vector<float> &texture_coords) = 0;
};

#endif //SHAPE_HPP
