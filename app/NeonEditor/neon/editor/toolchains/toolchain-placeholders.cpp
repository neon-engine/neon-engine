#include "toolchain-placeholders.hpp"

#include <algorithm>
#include <format>

namespace neon
{
  // Helpers of ToolchainPlaceholders: the walk over the braces of a text,
  // shared by the check and the filling.
  namespace
  {
    const std::vector<std::string> names = {"project", "build", "name", "engine", "platform"};

    std::string joined_names()
    {
      std::string joined;
      for (const auto &name : names)
      {
        if (!joined.empty()) { joined += ", "; }
        joined += "{" + name + "}";
      }
      return joined;
    }

    /// Walks the text, handing every placeholder to `on_placeholder` and
    /// every stretch between them to `on_text`. Returns false for a brace
    /// that is not closed, or a placeholder that is not known.
    template<typename OnText, typename OnPlaceholder>
    bool walk(const std::string &text, std::string &problem, OnText on_text, OnPlaceholder on_placeholder)
    {
      std::size_t start = 0;
      while (start < text.size())
      {
        const std::size_t open = text.find('{', start);
        if (open == std::string::npos)
        {
          on_text(text.substr(start));
          break;
        }
        on_text(text.substr(start, open - start));

        const std::size_t close = text.find('}', open);
        if (close == std::string::npos)
        {
          problem = std::format("the brace at '{}' is not closed", text.substr(open));
          return false;
        }

        const std::string name = text.substr(open + 1, close - open - 1);
        if (std::ranges::find(names, name) == names.end())
        {
          problem = std::format("'{{{}}}' is not a placeholder. There are: {}", name, joined_names());
          return false;
        }
        on_placeholder(name);
        start = close + 1;
      }
      return true;
    }

    /// Where a build goes, under the project, for a toolchain.
    std::string build_folder(const std::string &project, const std::string &toolchain)
    {
      return project + "/build/" + toolchain;
    }
  }

  const std::vector<std::string> &ToolchainPlaceholders::Names()
  {
    return names;
  }

  bool ToolchainPlaceholders::Check(const std::string &text, std::string &problem)
  {
    return walk(text, problem, [](const std::string &) {}, [](const std::string &) {});
  }

  ToolchainPlaceholders ToolchainPlaceholders::For(const Toolchain &toolchain, const BuildTarget &target)
  {
    if (toolchain.kind == ToolchainKind::Docker)
    {
      const std::string project(docker_project);
      return {
        .project = project,
        .build = build_folder(project, toolchain.name),
        .name = target.name,
        .engine = std::string(docker_engine),
        .platform = toolchain.platform
      };
    }

    return {
      .project = target.project,
      .build = build_folder(target.project, toolchain.name),
      .name = target.name,
      .engine = target.engine,
      .platform = toolchain.platform
    };
  }

  bool ToolchainPlaceholders::Fill(const std::string &text, std::string &filled, std::string &problem) const
  {
    std::string result;
    const bool whole = walk(
      text, problem,
      [&result](const std::string &piece) { result += piece; },
      [&result, this](const std::string &placeholder)
      {
        if (placeholder == "project") { result += project; }
        else if (placeholder == "build") { result += build; }
        else if (placeholder == "name") { result += name; }
        else if (placeholder == "engine") { result += engine; }
        else if (placeholder == "platform") { result += platform; }
      });
    if (!whole) { return false; }

    filled = result;
    return true;
  }
} // neon
