#ifndef API_VERSION_HPP
#define API_VERSION_HPP

#include <charconv>
#include <compare>
#include <string>
#include <string_view>

namespace neon
{
  /// The version of a graphics API, such as Vulkan 1.3. It is compared by
  /// its parts in order, so that 1.10 is above 1.9.
  struct ApiVersion
  {
    int major = 0;
    int minor = 0;

    /// What the driver adds to say which revision of the specification it
    /// follows. 0 where it is not known or does not matter, as in what is
    /// asked for.
    int patch = 0;

    auto operator<=>(const ApiVersion &) const = default;

    /// As it is written on the command line and in the log, such as `1.3`,
    /// with the patch only when there is one.
    [[nodiscard]] std::string ToString() const
    {
      std::string text = std::to_string(major) + "." + std::to_string(minor);
      if (patch != 0) { text += "." + std::to_string(patch); }
      return text;
    }

    /// Reads a version as it is written, such as `1.3`: two whole numbers
    /// with a dot between them. Returns false for anything else, and leaves
    /// `version` as it is.
    static bool Parse(const std::string_view text, ApiVersion &version)
    {
      const std::size_t dot = text.find('.');
      if (dot == std::string_view::npos) { return false; }

      const auto read = [](const std::string_view part, int &number)
      {
        const char *last = part.data() + part.size();
        const auto [stopped_at, error] = std::from_chars(part.data(), last, number);
        return !part.empty() && error == std::errc{} && stopped_at == last && number >= 0;
      };

      int major = 0;
      int minor = 0;
      if (!read(text.substr(0, dot), major) || !read(text.substr(dot + 1), minor)) { return false; }

      version = {major, minor};
      return true;
    }
  };
} // neon

#endif //API_VERSION_HPP
