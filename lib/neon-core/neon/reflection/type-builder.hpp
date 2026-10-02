#ifndef TYPE_BUILDER_HPP
#define TYPE_BUILDER_HPP

#include <algorithm>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "type-info.hpp"

namespace neon
{
  /// Writes the description of a type, field by field.
  ///
  /// A description is a function next to the type:
  ///
  ///     inline void Describe(TypeBuilder<Spectator> &type)
  ///     {
  ///       type.Named("Spectator");
  ///       type.Field("move_speed", &Spectator::move_speed).Describe("Units per second");
  ///       type.Field("look_speed", &Spectator::look_speed);
  ///     }
  ///
  /// What holds a field is deduced from the member: a `float` is a number,
  /// a `double` is a number that keeps its precision, a `std::string` is
  /// text, and so on. A member that is not described is
  /// not seen from outside, which is how a value that the engine keeps for
  /// itself stays out of a scene file.
  ///
  /// What follows a field, such as Describe() above, is about that field.
  template<typename T>
  class TypeBuilder
  {
    TypeInfo _type;

    // fields are added to the group that is open, or to the type
    std::vector<std::vector<FieldInfo> *> _open;

    FieldInfo *_last = nullptr;

    template<typename V>
    static FieldKind KindOf()
    {
      if constexpr (std::is_same_v<V, bool>) { return FieldKind::Bool; }
      else if constexpr (std::is_same_v<V, int>) { return FieldKind::Whole; }
      else if constexpr (std::is_same_v<V, float>) { return FieldKind::Number; }
      else if constexpr (std::is_same_v<V, std::string>) { return FieldKind::Text; }
      else if constexpr (std::is_same_v<V, glm::vec3>) { return FieldKind::Vector; }
      else if constexpr (std::is_same_v<V, Color>) { return FieldKind::Color; }
      else if constexpr (std::is_same_v<V, std::vector<std::string>>) { return FieldKind::TextList; }
      else if constexpr (std::is_same_v<V, FieldLength>) { return FieldKind::Length; }
      else if constexpr (std::is_same_v<V, std::vector<float>>) { return FieldKind::NumberList; }
      else if constexpr (std::is_same_v<V, double>) { return FieldKind::Precise; }
      else
      {
        // depends on V, so that it is only looked at for a type that gets here
        static_assert(
          sizeof(V) == 0,
          "A field of this type cannot be described. Use Choice() for an enum, Group() for a "
          "struct, or Field() with functions that turn it into a type that can.");
      }
    }

    FieldInfo &Add(const std::string &name, const FieldKind kind)
    {
      for (const auto &field : *_open.back())
      {
        if (field.name == name)
        {
          throw std::logic_error("Field '" + name + "' is described twice");
        }
      }

      auto &field = _open.back()->emplace_back();
      field.name = name;
      field.kind = kind;
      _last = &field;
      return field;
    }

    FieldInfo &Last()
    {
      if (_last == nullptr)
      {
        throw std::logic_error("There is no field yet that this could be about");
      }
      return *_last;
    }

  public:
    TypeBuilder()
    {
      _open.push_back(&_type.fields);
    }

    // it holds pointers into itself
    TypeBuilder(const TypeBuilder &) = delete;

    TypeBuilder &operator=(const TypeBuilder &) = delete;

    /// What the type is called from outside.
    TypeBuilder &Named(const std::string &name, const std::string &description = {})
    {
      _type.name = name;
      _type.description = description;
      return *this;
    }

    /// A field that is reached through a function, for a member of a member.
    template<typename V>
    TypeBuilder &Field(const std::string &name, const std::function<V &(T &)> &reach)
    {
      auto &field = Add(name, KindOf<V>());

      field.get = [reach](const void *object)
      {
        // reaching does not change the object
        return FieldValue{reach(*const_cast<T *>(static_cast<const T *>(object)))};
      };

      field.set = [reach](void *object, const FieldValue &value)
      {
        reach(*static_cast<T *>(object)) = std::get<V>(value);
      };

      return *this;
    }

    /// A field that is a member.
    template<typename V>
    TypeBuilder &Field(const std::string &name, V T::*member)
    {
      return Field<V>(name, [member](T &object) -> V & { return object.*member; });
    }

    /// A field that is reached through a function, written as a lambda.
    template<typename Reach>
      requires std::is_invocable_v<Reach, T &>
    TypeBuilder &Field(const std::string &name, Reach reach)
    {
      using V = std::remove_reference_t<std::invoke_result_t<Reach, T &>>;
      return Field<V>(name, std::function<V &(T &)>(reach));
    }

    /// A field that is kept as something else than it is seen as. `get`
    /// gives what is seen, and `set` takes it.
    template<typename V>
    TypeBuilder &Field(
      const std::string &name,
      const std::function<V(const T &)> &get,
      const std::function<void(T &, const V &)> &set)
    {
      auto &field = Add(name, KindOf<V>());

      field.get = [get](const void *object)
      {
        return FieldValue{get(*static_cast<const T *>(object))};
      };

      field.set = [set](void *object, const FieldValue &value)
      {
        set(*static_cast<T *>(object), std::get<V>(value));
      };

      return *this;
    }

    /// A field that is an enum. `choices` names its values, in their order,
    /// starting with the one that is 0.
    template<typename Reach>
      requires std::is_invocable_v<Reach, T &>
    TypeBuilder &Choice(const std::string &name, Reach reach, const std::vector<std::string> &choices)
    {
      using E = std::remove_reference_t<std::invoke_result_t<Reach, T &>>;
      static_assert(std::is_enum_v<E>, "A choice is an enum");

      auto &field = Add(name, FieldKind::Choice);
      field.choices = choices;

      field.get = [reach, choices](const void *object)
      {
        const auto index = static_cast<std::size_t>(reach(*const_cast<T *>(static_cast<const T *>(object))));
        return FieldValue{index < choices.size() ? choices[index] : std::string{}};
      };

      field.set = [reach, choices](void *object, const FieldValue &value)
      {
        const auto &word = std::get<std::string>(value);
        for (std::size_t i = 0; i < choices.size(); i++)
        {
          if (choices[i] == word) { reach(*static_cast<T *>(object)) = static_cast<E>(i); }
        }
      };

      return *this;
    }

    /// A field that is an enum and a member.
    template<typename E>
      requires std::is_enum_v<E>
    TypeBuilder &Choice(const std::string &name, E T::*member, const std::vector<std::string> &choices)
    {
      return Choice(name, [member](T &object) -> E & { return object.*member; }, choices);
    }

    /// A field that holds some of the 32 layers, one bit for each, the
    /// lowest for layer 1. It is seen as the numbers of its layers, so that
    /// layers 1 and 3 are `[1, 3]`.
    template<typename Reach>
      requires std::is_invocable_v<Reach, T &>
    TypeBuilder &Layers(const std::string &name, Reach reach)
    {
      static_assert(
        std::is_same_v<std::invoke_result_t<Reach, T &>, std::uint32_t &>,
        "Layers are kept in a std::uint32_t");

      auto &field = Add(name, FieldKind::Layers);

      field.get = [reach](const void *object)
      {
        const std::uint32_t bits = reach(*const_cast<T *>(static_cast<const T *>(object)));

        std::vector<float> layers;
        for (int layer = 1; layer <= 32; layer++)
        {
          if ((bits & (1u << (layer - 1))) != 0) { layers.push_back(static_cast<float>(layer)); }
        }
        return FieldValue{layers};
      };

      field.set = [reach](void *object, const FieldValue &value)
      {
        std::uint32_t bits = 0;
        for (const float layer : std::get<std::vector<float>>(value))
        {
          bits |= 1u << (static_cast<int>(layer) - 1);
        }
        reach(*static_cast<T *>(object)) = bits;
      };

      return *this;
    }

    /// A field that holds some of the 32 layers and is a member.
    TypeBuilder &Layers(const std::string &name, std::uint32_t T::*member)
    {
      return Layers(name, [member](T &object) -> std::uint32_t & { return object.*member; });
    }

    /// Fields that belong together under a name. `describe` adds them.
    TypeBuilder &Group(const std::string &name, const std::function<void(TypeBuilder &)> &describe)
    {
      auto &group = Add(name, FieldKind::Group);

      _open.push_back(&group.fields);
      describe(*this);
      _open.pop_back();

      // adding to a list moves what is in it, so the group is looked up again
      _last = &_open.back()->back();
      return *this;
    }

    /// What the fields have to be together. `check` says what is wrong with
    /// an object, or returns nothing, and is called once the fields of an
    /// object were read from a document. `where` is what the object is
    /// called in a message. What breaks a rule is kept as it was read.
    TypeBuilder &Rule(const std::function<std::string(const T &object, const std::string &where)> &check)
    {
      return Rule("", check);
    }

    /// A rule whose problem is reported at a field, with its line when the
    /// field is written.
    TypeBuilder &Rule(
      const std::string &field,
      const std::function<std::string(const T &object, const std::string &where)> &check)
    {
      if (_open.size() != 1) { throw std::logic_error("A rule is about the whole type, not about a group"); }

      _type.rules.push_back({field, [check](const void *object, const std::string &where)
      {
        return check(*static_cast<const T *>(object), where);
      }});

      // what follows a rule is about no field
      _last = nullptr;
      return *this;
    }

    /// What the field is for, in a sentence.
    TypeBuilder &Describe(const std::string &description)
    {
      Last().description = description;
      return *this;
    }

    TypeBuilder &Required()
    {
      Last().required = true;
      return *this;
    }

    TypeBuilder &AlwaysWritten()
    {
      Last().always_written = true;
      return *this;
    }

    TypeBuilder &OneNumberForAll()
    {
      Last().one_number_for_all = true;
      return *this;
    }

    TypeBuilder &Above(const float number)
    {
      Last().above = number;
      return *this;
    }

    TypeBuilder &AtLeast(const float number)
    {
      Last().at_least = number;
      return *this;
    }

    TypeBuilder &AtMost(const float number)
    {
      Last().at_most = number;
      return *this;
    }

    /// What a number counts, in the plural, such as `degrees`.
    TypeBuilder &Unit(const std::string &unit)
    {
      Last().unit = unit;
      return *this;
    }

    /// The field belongs to an object only while a choice holds one of the
    /// words. The choice is described before, in the same type or group,
    /// and it is read first.
    TypeBuilder &OnlyWhen(const std::string &choice, const std::vector<std::string> &words)
    {
      auto &field = Last();

      const FieldInfo *found = nullptr;
      for (const auto &known : *_open.back())
      {
        if (known.name == choice && known.kind == FieldKind::Choice) { found = &known; }
      }

      if (found == nullptr)
      {
        throw std::logic_error(
          "'" + field.name + "' depends on '" + choice + "', which is no choice described before it");
      }

      for (const auto &word : words)
      {
        if (std::ranges::find(found->choices, word) == found->choices.end())
        {
          throw std::logic_error("'" + word + "' is not a word of the choice '" + choice + "'");
        }
      }

      field.only_when = FieldCondition{choice, words};
      return *this;
    }

    /// How many numbers a list of numbers holds, when it is always as many.
    TypeBuilder &Count(const std::size_t count)
    {
      Last().count = count;
      return *this;
    }

    [[nodiscard]] TypeInfo Build() const
    {
      if (_type.name.empty())
      {
        throw std::logic_error("A description has to name its type, with Named()");
      }
      return _type;
    }
  };
} // neon

#endif //TYPE_BUILDER_HPP
