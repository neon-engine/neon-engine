#ifndef JOINT_KIND_HPP
#define JOINT_KIND_HPP

namespace neon
{
  /// How a joint holds two bodies together. The Joint component and the
  /// physics both read it.
  enum class JointKind
  {
    /// The two move as one. For something that is glued on.
    Fixed = 0,

    /// They turn around one axis through the anchor. A door, a wheel.
    Hinge,

    /// One moves along one axis through the anchor, and turns not at all. A
    /// drawer, a piston.
    Slider,

    /// They stay joined at the anchor and turn around it as they like. A
    /// pendulum, a chain.
    Point,

    /// Two points, one on each, stay within a length of each other and are
    /// free below it. A weight on a rope, a lamp on a chain, a dog on a
    /// lead.
    Rope
  };
} // neon

#endif //JOINT_KIND_HPP
