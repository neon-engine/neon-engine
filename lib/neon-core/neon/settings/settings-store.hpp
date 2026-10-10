#ifndef SETTINGS_STORE_HPP
#define SETTINGS_STORE_HPP

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <neon/data/data-value.hpp>
#include <neon/logging/logger.hpp>

#include "setting-declaration.hpp"
#include "settings-subscription.hpp"

namespace neon
{
  /// What a subscriber is told: the name of the setting and what it holds
  /// now, after it was stored.
  using SettingsCallback = std::function<void(const std::string &name, const DataValue &value)>;

  /// The settings of a game, by name: each declared with its kind, its
  /// default, and what it may be set to, see SettingDeclaration, and what
  /// it holds now. A game declares them in its settings file under `game`
  /// (SettingsFile), the player's file sets them, the settings menu changes
  /// them (GameMenu), the scripts read and set them (`settings` in Lua),
  /// and whoever cares hears of a change through OnChange().
  ///
  /// A value that does not fit the kind, the range, or the choices is
  /// refused, which is said in the log once for a name. A change notifies
  /// every subscriber of the name after the value is stored; a value set
  /// to what it is already notifies nobody.
  ///
  /// The store knows no backend and no menu: it is a plain class. The
  /// engine's own settings join it later (#542).
  class SettingsStore final
  {
    /// A setting, with what it holds now.
    struct Setting
    {
      SettingDeclaration declaration;
      DataValue value;
    };

    /// Who hears of a change of a name.
    struct Subscriber
    {
      int id = 0;
      std::string name;
      SettingsCallback callback;
    };

    std::shared_ptr<Logger> _logger;
    std::vector<Setting> _settings;
    std::vector<Subscriber> _subscribers;
    int _next_subscriber = 1;

    // how deep the notifications are, and whether one ended a subscription
    // meanwhile, which is swept once they are done
    int _notifying = 0;
    bool _ended_while_notifying = false;

    // the names refused once, which are not said again
    std::unordered_set<std::string> _refused;

    // outlives the store in the subscriptions, which ask it whether the
    // store is still there
    std::shared_ptr<int> _token = std::make_shared<int>(0);

    Setting *FindSetting(const std::string &name);

    [[nodiscard]] const Setting *FindSetting(const std::string &name) const;

    /// Says why a value was refused, once for a name.
    void Refuse(const std::string &name, const std::string &why);

    void Notify(const std::string &name, const DataValue &value);

  public:
    explicit SettingsStore(const std::shared_ptr<Logger> &logger);

    /// Whether a text is a name a setting may have: letters, digits, and
    /// underscores, not empty.
    [[nodiscard]] static bool IsName(const std::string &name);

    /// Whether `value` is what a setting of this declaration may hold, and
    /// why not in `why`: a flag for a flag, a number within the range for a
    /// number, a text for a text, and one of the choices for a choice.
    [[nodiscard]] static bool Fits(const SettingDeclaration &declaration, const DataValue &value, std::string &why);

    /// Declares a setting, which then holds its `value`, or its default
    /// when that is empty. Returns false, and says why in the log, for a
    /// name that is taken or is no name, a choice without choices, or a
    /// default that does not fit.
    bool Declare(const SettingDeclaration &declaration);

    [[nodiscard]] bool Has(const std::string &name) const;

    /// The declaration of a name, or null.
    [[nodiscard]] const SettingDeclaration *Find(const std::string &name) const;

    /// What a name holds, or null for a name that is not declared.
    [[nodiscard]] const DataValue *Get(const std::string &name) const;

    /// What a name holds, as its kind. Each returns false and leaves the
    /// value alone for a name that is not declared or holds another kind.
    [[nodiscard]] bool GetFlag(const std::string &name, bool &flag) const;

    [[nodiscard]] bool GetNumber(const std::string &name, double &number) const;

    [[nodiscard]] bool GetText(const std::string &name, std::string &text) const;

    /// Sets what a name holds and tells the subscribers of the name, unless
    /// it holds that already. Returns false for a value that does not fit,
    /// or a name that is not declared, which is said in the log once for a
    /// name. An action holds nothing and is not set, see Trigger().
    bool Set(const std::string &name, const DataValue &value);

    /// Tells the subscribers of an action that it was pressed, with no
    /// value. Returns false for a name that is not an action, which is said
    /// in the log once for a name.
    bool Trigger(const std::string &name);

    /// The names of the settings, in the order they were declared in.
    [[nodiscard]] std::vector<std::string> Names() const;

    /// Calls `callback` after the setting of that name changes, for as long
    /// as the subscription lives. A name that is not declared is logged,
    /// and the subscription is one that hears nothing.
    [[nodiscard]] SettingsSubscription OnChange(const std::string &name, const SettingsCallback &callback);

    /// For SettingsSubscription: ends the subscription of that id.
    void Unsubscribe(int id);
  };
} // neon

#endif //SETTINGS_STORE_HPP
