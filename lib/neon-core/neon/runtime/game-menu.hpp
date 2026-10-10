#ifndef GAME_MENU_HPP
#define GAME_MENU_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/data/data-value.hpp>
#include <neon/logging/logger.hpp>
#include <neon/settings/player-settings.hpp>
#include <neon/settings/settings-store.hpp>
#include <neon/ui/ui-context.hpp>

namespace neon
{
  /// The settings of the game in a settings menu: every setting the game
  /// declared, see SettingsStore, is a value of the user interface named as
  /// the setting is, which an element of the menu follows (`checked:
  /// "{subtitles}"`, `value: "{difficulty}"`). The game writes no code for
  /// it. While the menu is shown, what the player changes is set on the
  /// store at once, which tells the subscribers; what they keep is written
  /// to user://settings.yml under `game` when the menu is closed with
  /// Apply, and what they did not keep is put back as it was.
  ///
  /// The menu builds its rows as well: when it is opened with the document
  /// of the engine's settings menu, every setting that no element of the
  /// document is named after is made inside the element named `list`, in
  /// a section per `category`, under a heading per `group`, in the `order`
  /// of the declarations, with the `control` each declares, see
  /// SettingDeclaration. A row the recipe writes, named after a setting,
  /// binds as before and is not made again, so a game with a menu of its
  /// own writes its rows by hand where it wants to.
  ///
  /// It follows GraphicsMenu, which does the same for the graphics of the
  /// engine, and the two are driven side by side by the runtime until the
  /// engine's settings join the store (#542).
  class GameMenu final
  {
    /// A setting the menu shows: what the menu showed when it was opened,
    /// what was set last, and what was asked for last and could not be
    /// taken, as the menu shows it.
    struct Row
    {
      std::string name;
      DataValue opened;
      DataValue applied;
      std::string refused;
    };

    UiContext *_ui;
    SettingsStore *_store;
    PlayerSettings *_player;
    std::shared_ptr<Logger> _logger;

    std::vector<Row> _rows;
    bool _is_open = false;

    /// Shows a value as the element that follows it needs it: a flag as a
    /// flag, a number as a number, a text as a text.
    void Show(const std::string &name, const DataValue &value) const;

    /// Reads what the user interface shows under a name as a value of the
    /// setting's kind, or returns false for a text that is none.
    [[nodiscard]] bool Read(const Row &row, const std::string &shown, DataValue &value) const;

    /// Makes the sections, headings, and rows of the settings the document
    /// has no element for, inside its element named `list`. Without one
    /// nothing is made.
    void Build(int document) const;

    /// The description of a row of a setting, as a file would write it.
    [[nodiscard]] static DataValue DescribeRow(const SettingDeclaration &declaration);

  public:
    /// The player's settings may be null, and nothing is kept then.
    GameMenu(UiContext *ui, SettingsStore *store, PlayerSettings *player, const std::shared_ptr<Logger> &logger);

    /// The menu was shown as `document`: its rows are built, see above,
    /// and its values are set to what the settings hold. Without a document
    /// nothing is built, and the values are set alone.
    void Open(int document = -1);

    /// Sets on the store what the player changed since the last frame,
    /// and triggers an action whose button was pressed. What cannot be
    /// taken is said once, and the menu goes on showing it.
    void Update();

    /// The menu was closed. With `keep`, what changed since it was opened
    /// is written to the file of the player; without it, it is put back.
    void Close(bool keep);

    [[nodiscard]] bool IsOpen() const { return _is_open; }

    /// The names of the values the menu reads: the settings of the game.
    [[nodiscard]] std::vector<std::string> GetValueNames() const;
  };
} // neon

#endif //GAME_MENU_HPP
