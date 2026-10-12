#ifndef TOOLCHAIN_KIND_HPP
#define TOOLCHAIN_KIND_HPP

#include <string>
#include <string_view>

namespace neon
{
  /// Where the programs of a toolchain run: on this machine, or in a
  /// container of a Docker image, with the project mounted into it.
  enum class ToolchainKind
  {
    Native,
    Docker
  };

  /// The word of a kind as it is written in the file: `native` or `docker`.
  [[nodiscard]] inline std::string_view toolchain_kind_name(const ToolchainKind kind)
  {
    return kind == ToolchainKind::Docker ? "docker" : "native";
  }

  /// Reads the word of a kind. Returns false for any other word.
  [[nodiscard]] inline bool toolchain_kind_of(const std::string &name, ToolchainKind &kind)
  {
    if (name == "native")
    {
      kind = ToolchainKind::Native;
      return true;
    }
    if (name == "docker")
    {
      kind = ToolchainKind::Docker;
      return true;
    }
    return false;
  }
} // neon

#endif //TOOLCHAIN_KIND_HPP
