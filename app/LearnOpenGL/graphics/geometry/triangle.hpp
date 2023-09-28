#ifndef TRIANGLE_HPP
#define TRIANGLE_HPP

#include "shape.hpp"

class Triangle : public Shape {
 public:
  Triangle();
  ~Triangle();
  void Draw() const override;
};

#endif //TRIANGLE_HPP
