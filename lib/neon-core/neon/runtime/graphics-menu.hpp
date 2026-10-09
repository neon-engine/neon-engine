#ifndef GRAPHICS_MENU_HPP
#define GRAPHICS_MENU_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <neon/data/data-value.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-context.hpp>
#include <neon/settings/player-settings.hpp>
#include <neon/ui/ui-context.hpp>
#include <neon/window/window-context.hpp>

namespace neon
{
  /// The graphics of a settings menu: every graphics setting the engine has
  /// is a value of the user interface, named as the setting is, which an
  /// element of the menu follows (`checked: "{vsync}"`, `value:
  /// "{anisotropy}"`). While the menu is shown, what the player changes
  /// takes effect at once; what they keep is written to user://settings.yml
  /// when the menu is closed with Apply, and what they did not keep is put
  /// back as it was.
  ///
  /// | Value | Setting | Takes |
  /// |---|---|---|
  /// | `window_mode` | `window.mode` | `windowed`, `borderless`, `fullscreen` |
  /// | `vsync` | `rendering.vsync` | a flag |
  /// | `max_fps` | `rendering.max_fps` | 0, or 30 to 300 |
  /// | `anisotropy` | `rendering.anisotropy` | 1, 2, 4, 8, 16 |
  /// | `texture_scale` | `rendering.texture_scale` | 1, 0.5, 0.25, 0.125, for the textures read from then on |
  /// | `target_scale` | `rendering.target_scale` | 1, 0.5, 0.25, for the targets made from then on |
  /// | `target_mipmaps` | `rendering.target_mipmaps` | 0 to 16, for the targets made from then on |
  /// | `shadows` | `rendering.shadows` | a flag: off skips the shadow pass |
  /// | `shadow_map_size` | `rendering.shadow_map_size` | 512, 1024, 2048, 4096; the map is made again |
  /// | `shadow_filter` | `rendering.shadow_filter` | `none`, `pcf` |
  /// | `shadow_cascades` | `rendering.shadow_cascades` | 1 to 4 |
  /// | `shadow_distance` | `rendering.shadow_distance` | meters, above 0 |
  /// | `tonemapper` | `rendering.tonemapper` | `none`, `aces`, `agx` |
  /// | `exposure` | `rendering.exposure` | above 0 |
  class GraphicsMenu final
  {
    /// A setting the menu changes: the value of the user interface, where
    /// the setting is written, what it is now, and how it is changed.
    struct Setting
    {
      std::string value;
      std::string section;
      std::string name;

      // what it is now, as the text the menu shows
      std::function<std::string()> current;

      // changes it to what the text says, or returns false
      std::function<bool(const std::string &)> apply;

      // the text as it is written to the file of the player
      std::function<DataValue(const std::string &)> kept;

      // what the menu showed when it was opened, what was applied last, and
      // what was asked for last and could not be taken
      std::string opened;
      std::string applied;
      std::string refused;
    };

    UiContext *_ui;
    RenderContext *_render;
    WindowContext *_window;
    PlayerSettings *_player;
    std::shared_ptr<Logger> _logger;

    std::vector<Setting> _settings;
    bool _is_open = false;

    void Describe();

  public:
    /// The player's settings may be null, and nothing is kept then.
    GraphicsMenu(
      UiContext *ui,
      RenderContext *render,
      WindowContext *window,
      PlayerSettings *player,
      const std::shared_ptr<Logger> &logger);

    /// The menu was shown: its values are set to what the graphics are now.
    void Open();

    /// Takes effect what the player changed since the last frame. What
    /// cannot be taken is logged, and the menu shows what holds again.
    void Update();

    /// The menu was closed. With `keep`, what changed since it was opened is
    /// written to the file of the player; without it, it is put back.
    void Close(bool keep);

    [[nodiscard]] bool IsOpen() const { return _is_open; }

    /// The names of the values the menu reads, in the order of the table.
    [[nodiscard]] std::vector<std::string> GetValueNames() const;
  };
} // neon

#endif //GRAPHICS_MENU_HPP
