#ifndef AUDIO_MENU_HPP
#define AUDIO_MENU_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/audio/audio-context.hpp>
#include <neon/audio/sound-group-setting.hpp>
#include <neon/logging/logger.hpp>
#include <neon/settings/player-settings.hpp>
#include <neon/ui/ui-context.hpp>

namespace neon
{
  /// The volumes of a settings menu: every group of sounds, those of the
  /// engine and those the project declares, is a value of the user
  /// interface named after the group, from 0 to 100, which a slider of the
  /// menu follows (`value: "{music}"`). When the menu is shown the values are
  /// set to the volumes the groups have; while it is shown, what the player
  /// changes is heard at once; what they keep is written to
  /// user://settings.yml, as `audio.volumes`, when the menu is closed with
  /// Apply, and what they did not keep is put back as it was.
  ///
  /// | Value | Setting | Takes |
  /// |---|---|---|
  /// | `music`, `effects`, `voices`, `ambience`, and a group of the project | `audio.volumes.<group>` | 0 to 100, for a volume of 0 to 1. Below 0 is silence |
  class AudioMenu final
  {
    /// A group the menu changes the volume of.
    struct Volume
    {
      std::string group;

      // the volume of the group when the menu was opened, from 0 to 1, to
      // put back as it was
      float opened = 1.0f;

      // the value the menu showed when it was opened, and the value that
      // was heard last, from 0 to 100
      double shown = 0.0;
      double applied = 0.0;
    };

    UiContext *_ui;
    AudioContext *_audio;
    PlayerSettings *_player;
    std::shared_ptr<Logger> _logger;

    std::vector<Volume> _volumes;
    bool _is_open = false;

  public:
    /// What a value is at the full volume of its group.
    static constexpr double kFull = 100.0;

    /// The player's settings may be null, and nothing is kept then.
    AudioMenu(
      UiContext *ui,
      AudioContext *audio,
      const std::vector<SoundGroupSetting> &groups,
      PlayerSettings *player,
      const std::shared_ptr<Logger> &logger);

    /// The menu was shown: its values are set to the volumes as they are.
    void Open();

    /// Hands the audio what the player changed since the last frame.
    void Update();

    /// The menu was closed. With `keep`, what changed since it was opened is
    /// written to the file of the player; without it, it is put back.
    void Close(bool keep);

    [[nodiscard]] bool IsOpen() const { return _is_open; }

    /// The names of the values the menu reads, which are the groups, in the
    /// order the settings list them.
    [[nodiscard]] std::vector<std::string> GetValueNames() const;
  };
} // neon

#endif //AUDIO_MENU_HPP
