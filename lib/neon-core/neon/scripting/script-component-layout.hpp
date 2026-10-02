#ifndef SCRIPT_COMPONENT_LAYOUT_HPP
#define SCRIPT_COMPONENT_LAYOUT_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <neon/reflection/type-info.hpp>
#include <neon/world-system/ecs/component-info.hpp>
#include <neon/world-system/ecs/scene-file/component-format.hpp>

#include "script-field.hpp"

namespace neon
{
  /// How a component a script declares lies in memory, and everything the
  /// engine needs to treat it as one of its own: how the store creates,
  /// copies, and ends it, how it is described, and how a recipe reads and
  /// writes it.
  ///
  /// The fields are laid out one after the other, each at the alignment of
  /// what it holds, so a system reads them as it reads a C++ struct. The
  /// layout is the same whichever language declared the component, which is
  /// what lets a second language come later behind the same interface.
  class ScriptComponentLayout
  {
    /// Where one field lies.
    struct Slot
    {
      FieldKind kind = FieldKind::Number;
      std::size_t offset = 0;
      FieldValue standard;
    };

    /// What the functions of the component share.
    struct Shape
    {
      std::string name;
      std::size_t size = 0;
      std::size_t alignment = 1;
      std::vector<Slot> slots;
    };

    std::shared_ptr<const Shape> _shape;
    std::shared_ptr<const TypeInfo> _type;
    std::string _problem;

    /// Creates the fields of one component in memory that holds none.
    static void Construct(const Shape &shape, void *at);

    static void Destruct(const Shape &shape, void *at);

    static void Copy(const Shape &shape, void *to, const void *from);

    static void Move(const Shape &shape, void *to, void *from);

    [[nodiscard]] static FieldValue Get(const Slot &slot, const void *object);

    static void Set(const Slot &slot, void *object, const FieldValue &value);

  public:
    /// Whether a field of this kind can be part of a script's component. A
    /// bool, a whole number, a number, a precise number, text, a vector,
    /// and a colour can; the rest is open.
    [[nodiscard]] static bool Supports(FieldKind kind);

    /// Lays out the fields in the order given. What is wrong with them, if
    /// anything, is in GetProblem() afterwards, and the layout is then not
    /// to be used.
    ScriptComponentLayout(
      const std::string &name,
      const std::string &description,
      const std::vector<ScriptField> &fields);

    /// What is wrong with the fields, or empty.
    [[nodiscard]] const std::string &GetProblem() const;

    [[nodiscard]] const std::string &GetName() const;

    [[nodiscard]] std::size_t GetSize() const;

    [[nodiscard]] std::size_t GetAlignment() const;

    /// What the store registers.
    [[nodiscard]] ComponentInfo GetComponentInfo() const;

    /// What reads and changes the fields by name.
    [[nodiscard]] std::shared_ptr<const TypeInfo> GetTypeInfo() const;

    /// How a recipe reads and writes the component.
    [[nodiscard]] ComponentFormat GetComponentFormat() const;
  };
} // neon

#endif //SCRIPT_COMPONENT_LAYOUT_HPP
