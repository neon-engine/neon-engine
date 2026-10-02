#ifndef JOINT_STATE_HPP
#define JOINT_STATE_HPP

namespace neon
{
  /// Where a hinge or a slider is, read back from the physics. A fixed
  /// joint and a point joint have no state, and read as zeros.
  struct JointState
  {
    /// Of a hinge, how far it turned from where it was made, in radians
    /// around the axis. Of a slider, how far it moved along the axis, in
    /// units. Within the limits, when there are any.
    float position = 0.0f;

    /// How fast that changes: radians per second, or units per second.
    float velocity = 0.0f;
  };
} // neon

#endif //JOINT_STATE_HPP
