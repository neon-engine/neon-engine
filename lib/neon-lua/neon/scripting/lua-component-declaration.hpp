#ifndef LUA_COMPONENT_DECLARATION_HPP
#define LUA_COMPONENT_DECLARATION_HPP

#include <string>
#include <vector>

#include <neon/scripting/script-field.hpp>

namespace neon
{
  /// What a script declared with `Component:extend`, read off its class
  /// table: the name, and the fields with their defaults, in the order of
  /// their names, so that the layout is the same on every run.
  struct LuaComponentDeclaration
  {
    std::string name;

    std::vector<ScriptField> fields;
  };
} // neon

#endif //LUA_COMPONENT_DECLARATION_HPP
