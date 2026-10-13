#include "toolchains.hpp"

#include <algorithm>

namespace neon
{
  Toolchains Toolchains::Defaults()
  {
    Toolchains defaults;
    defaults.Set(Toolchain{
      .name = "macos-arm64",
      .kind = ToolchainKind::Native,
      .platform = "darwin-arm64",
      .required = {"cmake", "ninja", "/opt/homebrew/opt/llvm@20/bin/clang++"},
      .configure = "cmake -S {project} -B {build} -DNEON_RUNTIME_DIRECTORY={engine}/bin/release/darwin-arm64/NeonRuntime",
      .build = "cmake --build {build}",
      .run = "{build}/{name}/{name}"
    });
    defaults.Set(Toolchain{
      .name = "linux-x64",
      .kind = ToolchainKind::Docker,
      .platform = "linux-x86_64",
      .image = "neon-engine/linux-x64",
      .required = {"docker"},
      .configure = "cmake -S {project} -B {build} -DNEON_RUNTIME_DIRECTORY={engine}/bin/release/linux-x86_64/NeonRuntime",
      .build = "cmake --build {build}"
    });
    defaults.Set(Toolchain{
      .name = "windows-x64",
      .kind = ToolchainKind::Docker,
      .platform = "windows-x86_64",
      .image = "neon-engine/windows-x64",
      .required = {"docker"},
      .configure = "cmake -S {project} -B {build} -DCMAKE_TOOLCHAIN_FILE={engine}/cmake/toolchains/windows-x64-llvm-mingw.cmake "
                   "-DNEON_RUNTIME_DIRECTORY={engine}/bin/release/windows-x86_64/NeonRuntime",
      .build = "cmake --build {build}"
    });
    return defaults;
  }

  void Toolchains::Set(const Toolchain &toolchain)
  {
    const auto existing = std::ranges::find(_toolchains, toolchain.name, &Toolchain::name);
    if (existing != _toolchains.end())
    {
      *existing = toolchain;
      return;
    }
    _toolchains.push_back(toolchain);
  }

  void Toolchains::Drop(const std::string &name)
  {
    std::erase_if(_toolchains, [&name](const Toolchain &toolchain) { return toolchain.name == name; });
    if (std::ranges::find(_dropped, name) == _dropped.end()) { _dropped.push_back(name); }
  }

  void Toolchains::SetEditor(const std::string &name, const std::string &path)
  {
    _editors[name] = path;
  }

  const Toolchain *Toolchains::Find(const std::string &name) const
  {
    const auto found = std::ranges::find(_toolchains, name, &Toolchain::name);
    return found == _toolchains.end() ? nullptr : &*found;
  }

  const std::vector<Toolchain> &Toolchains::All() const
  {
    return _toolchains;
  }

  const std::vector<std::string> &Toolchains::Dropped() const
  {
    return _dropped;
  }

  std::string Toolchains::Editor(const std::string &name) const
  {
    const auto found = _editors.find(name);
    return found == _editors.end() ? "" : found->second;
  }

  const std::map<std::string, std::string> &Toolchains::Editors() const
  {
    return _editors;
  }

  void Toolchains::Apply(const Toolchains &over)
  {
    for (const auto &name : over._dropped) { Drop(name); }
    for (const auto &toolchain : over._toolchains) { Set(toolchain); }
    for (const auto &[name, path] : over._editors) { SetEditor(name, path); }
  }
} // neon
