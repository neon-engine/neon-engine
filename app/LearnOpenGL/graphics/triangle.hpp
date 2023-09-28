#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "geometry.hpp"

class Triangle : public Geometry {
 public:
  Triangle();
  ~Triangle();
  void Draw() const override;
};

#endif //TRIANGLE_HPP
