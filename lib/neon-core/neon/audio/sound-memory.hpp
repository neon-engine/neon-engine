#ifndef SOUND_MEMORY_HPP
#define SOUND_MEMORY_HPP

#include <string>
#include <string_view>

namespace neon
{
  /// The paths that name a sound that was handed to the audio in memory.
  ///
  /// A sound is named by a path everywhere, in a recipe, a prefab, a field.
  /// Most paths name a file, `assets://sounds/step.wav`. A path that starts
  /// with `sound://` names the bytes of a file that were handed over under
  /// a name, with AudioContext::SetSound(), by an extension that reads them
  /// out of an archive of its own. It is no scheme of the file system,
  /// which neither reads nor lists it, see docs/file-systems.md.
  ///
  /// This is the one place it is written down, as TextureSource is for the
  /// pictures in memory.
  struct SoundMemory
  {
    static constexpr std::string_view scheme = "sound://";

    /// Whether a path names a sound in memory, and not a file.
    [[nodiscard]] static bool IsNamedBy(const std::string_view path)
    {
      return path.starts_with(scheme) && path.size() > scheme.size();
    }

    /// The name a path of a sound in memory carries. Empty for a path that
    /// names a file.
    [[nodiscard]] static std::string NameOf(const std::string_view path)
    {
      return IsNamedBy(path) ? std::string(path.substr(scheme.size())) : std::string();
    }

    /// The path that names the sound of a name.
    [[nodiscard]] static std::string For(const std::string_view name)
    {
      return std::string(scheme) + std::string(name);
    }
  };
} // neon

#endif //SOUND_MEMORY_HPP
