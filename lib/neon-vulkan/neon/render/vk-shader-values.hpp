#ifndef VK_SHADER_VALUES_HPP
#define VK_SHADER_VALUES_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace neon
{
  /// A value a shader declares, and where it lies in the block the shader
  /// reads its values from.
  // ReSharper disable once CppInconsistentNaming
  struct VK_ShaderValue
  {
    std::string name;

    /// In bytes from the start of the block.
    std::uint32_t offset = 0;

    /// How many numbers it holds: 1 for a number, up to 4 for a vector.
    std::uint32_t count = 1;

    /// Whether the numbers are whole numbers.
    bool is_integer = false;
  };

  /// The values a shader declares in its block `Values`:
  ///
  ///     layout (set = 2, binding = 0) uniform Values
  ///     {
  ///         float intensity;
  ///         vec4 tint;
  ///     } values;
  ///
  /// They are read from the compiled shader, which holds the names and the
  /// places of the members of a block. A file of a user interface can then
  /// set a value by its name, and nothing has to be written down twice.
  // ReSharper disable once CppInconsistentNaming
  struct VK_ShaderValues
  {
    /// What the block has to be called in the shader.
    static constexpr const char *kBlock_Name = "Values";

    std::vector<VK_ShaderValue> values;

    /// The size of the block in bytes. 0 for a shader without values.
    std::uint32_t size = 0;

    [[nodiscard]] const VK_ShaderValue *Find(const std::string &name) const;

    /// Reads the block from SPIR-V. Returns false for what is no SPIR-V. A
    /// shader without the block has no values, which is no mistake.
    /// Members that are neither numbers nor vectors of numbers are left
    /// out.
    [[nodiscard]] static bool Read(const std::vector<std::uint32_t> &words, VK_ShaderValues &values);
  };
} // neon

#endif //VK_SHADER_VALUES_HPP
