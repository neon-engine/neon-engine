#ifndef API_VERSION_HPP
#define API_VERSION_HPP

#include <compare>
#include <string>

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
  };
} // neon

#endif //API_VERSION_HPP
