#ifndef CUBE_HPP
#define CUBE_HPP
#include "geometry.hpp"


class Cube final : public Geometry {
public:
    Cube();
    ~Cube();
    void Draw() const override;
};



#endif //CUBE_HPP
