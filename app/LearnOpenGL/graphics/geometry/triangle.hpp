#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "shape.hpp"

class Triangle : public IShape {
 public:
  Triangle(
      const std::vector<float> &vertices,
      const std::vector<int> &indices,
      const std::vector<float> &texture_coords
      ) :  {}
};

#endif //TRIANGLE_HPP
