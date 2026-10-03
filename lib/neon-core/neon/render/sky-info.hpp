#ifndef SKY_INFO_HPP
#define SKY_INFO_HPP

#include <string>

#include "sky-type.hpp"

namespace neon
{
  /// What is seen behind everything else in a scene, in every direction
  /// and endlessly far away: the camera turns in it and never moves
  /// through it.
  struct SkyInfo
  {
    SkyType type = SkyType::Box;

    /// For a box: the virtual paths of the six images, each named after
    /// the direction it is seen in. `front` is what a camera that was not
    /// turned looks at, which is along negative z, `right` is along
    /// positive x, and `top` along positive y. The four sides are upright,
    /// the lower edge of `top` meets the upper edge of `front`, and the
    /// upper edge of `bottom` meets the lower edge of `front`: the cross
    /// that sky boxes are painted as, folded around the camera. All six are
    /// squares of one size.
    std::string right;
    std::string left;
    std::string top;
    std::string bottom;
    std::string front;
    std::string back;

    /// For a sphere: the virtual path of one panorama, twice as wide as
    /// high, that holds every direction (an equirectangular image). Its
    /// middle is seen along negative z, its upper edge is straight up.
    std::string texture;

    /// Degrees the sky is turned around the direction that is up, seen
    /// from above against the clock.
    float rotation = 0.0f;

    /// What the light of the images is multiplied by. 1 shows them as they
    /// are.
    float brightness = 1.0f;
  };
} // neon

#endif //SKY_INFO_HPP
