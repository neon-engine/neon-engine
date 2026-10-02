#ifndef DOUBLE_SIDED_HPP
#define DOUBLE_SIDED_HPP

namespace neon
{
  /// Whether both sides of every triangle of a material are drawn. Most
  /// surfaces are seen from one side only, the outside of a closed model,
  /// so the back is left out and the graphics card skips it. A leaf, a
  /// flag, or a sheet of glass that is seen from both sides draws both.
  enum class DoubleSided
  {
    /// What the model file says: `doubleSided` of a glTF material. One
    /// side for a file that says nothing, and for a mesh that was built.
    Model = 0,

    /// Both sides, whatever the file says.
    Always,

    /// One side, whatever the file says.
    Never
  };
} // neon

#endif //DOUBLE_SIDED_HPP
