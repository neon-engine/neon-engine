#ifndef PLANE_HPP
#define PLANE_HPP

#include "geometry.hpp"

class Plane final : public Geometry
{
public:
  Plane();

  ~Plane();

  void Draw() const override;
};

#endif //PLANE_HPP
