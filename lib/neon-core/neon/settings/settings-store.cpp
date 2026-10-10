#include "settings-store.hpp"

#include <algorithm>
#include <cctype>
#include <format>

namespace neon
{
  // Helpers of SettingsStore: comparing values, and naming them in a message.
  namespace
  {
    bool same(const DataValue &a, const DataValue &b)
    {
      if (a.GetKind() != b.GetKind()) { return false; }

      switch (a.GetKind())
      {
        case DataValue::Kind::Bool:
        {
          bool x = false;
          bool y = false;
          return a.GetBool(x) && b.GetBool(y) && x == y;
        }
        case DataValue::Kind::Number:
        {
          double x = 0.0;
          double y = 0.0;
          return a.GetNumber(x) && b.GetNumber(y) && x == y;
        }
        case DataValue::Kind::Text:
        {
          std::string x;
          std::string y;
          return a.GetText(x) && b.GetText(y) && x == y;
        }
        default: return false;
      }
    }

    /// A value as a message shows it.
    std::string written(const DataValue &value)
    {
      bool flag = false;
      double number = 0.0;
      std::string text;
      if (value.GetBool(flag)) { return flag ? "true" : "false"; }
      if (value.GetNumber(number)) { return std::format("{}", number); }
      if (value.GetText(text)) { return std::format("'{}'", text); }
      return DataValue::Describe(value.GetKind());
    }

    std::string list_of(const std::vector<std::string> &names)
    {
      std::string list;
      for (const auto &name : names)
      {
        if (!list.empty()) { list += ", "; }
        list += name;
      }
      return list;
    }
  }

  SettingsStore::SettingsStore(const std::shared_ptr<Logger> &logger)
  {
    _logger = logger;
  }

  bool SettingsStore::IsName(const std::string &name)
  {
    if (name.empty()) { return false; }
    return std::ranges::all_of(name, [](const unsigned char c) { return std::isalnum(c) != 0 || c == '_'; });
  }

  bool SettingsStore::Fits(const SettingDeclaration &declaration, const DataValue &value, std::string &why)
  {
    switch (declaration.kind)
    {
      case SettingKind::Flag:
      {
        bool flag = false;
        if (value.GetBool(flag)) { return true; }
        why = std::format("{} is {}, where true or false was expected", written(value), DataValue::Describe(value.GetKind()));
        return false;
      }
      case SettingKind::Number:
      {
        double number = 0.0;
        if (!value.GetNumber(number))
        {
          why = std::format("{} is {}, where a number was expected", written(value), DataValue::Describe(value.GetKind()));
          return false;
        }
        if (declaration.least.has_value() && number < *declaration.least)
        {
          why = std::format("{} is below the least, which is {}", number, *declaration.least);
          return false;
        }
        if (declaration.most.has_value() && number > *declaration.most)
        {
          why = std::format("{} is above the most, which is {}", number, *declaration.most);
          return false;
        }
        return true;
      }
      case SettingKind::Text:
      {
        std::string text;
        if (value.GetText(text)) { return true; }
        why = std::format("{} is {}, where a text was expected", written(value), DataValue::Describe(value.GetKind()));
        return false;
      }
      case SettingKind::Choice:
      {
        std::string text;
        if (!value.GetText(text))
        {
          why = std::format(
            "{} is {}, where one of the choices was expected: {}",
            written(value), DataValue::Describe(value.GetKind()), list_of(declaration.choices));
          return false;
        }
        if (std::ranges::find(declaration.choices, text) != declaration.choices.end()) { return true; }
        why = std::format("'{}' is none of the choices, which are: {}", text, list_of(declaration.choices));
        return false;
      }
      case SettingKind::Action:
      {
        if (value.IsEmpty()) { return true; }
        why = std::format("an action holds nothing, and {} was given", written(value));
        return false;
      }
    }
    why = "the kind is not known";
    return false;
  }

  SettingsStore::Setting *SettingsStore::FindSetting(const std::string &name)
  {
    const auto it = std::ranges::find(_settings, name, [](const Setting &setting) { return setting.declaration.name; });
    return it == _settings.end() ? nullptr : &*it;
  }

  const SettingsStore::Setting *SettingsStore::FindSetting(const std::string &name) const
  {
    const auto it = std::ranges::find(_settings, name, [](const Setting &setting) { return setting.declaration.name; });
    return it == _settings.end() ? nullptr : &*it;
  }

  bool SettingsStore::Declare(const SettingDeclaration &declaration)
  {
    if (!IsName(declaration.name))
    {
      _logger->Error("'{}' is not the name of a setting: letters, digits, and underscores were expected", declaration.name);
      return false;
    }
    if (Has(declaration.name))
    {
      _logger->Error("The setting {} is declared already", declaration.name);
      return false;
    }
    if (declaration.kind == SettingKind::Choice && declaration.choices.empty())
    {
      _logger->Error("The setting {} is a choice without choices", declaration.name);
      return false;
    }

    std::string why;
    if (!Fits(declaration, declaration.default_value, why))
    {
      _logger->Error("The default of the setting {} does not fit: {}", declaration.name, why);
      return false;
    }

    DataValue value = declaration.value;
    if (value.IsEmpty()) { value = declaration.default_value; }
    else if (!Fits(declaration, value, why))
    {
      _logger->Warn("The setting {} starts with its default, since what it was given does not fit: {}", declaration.name, why);
      value = declaration.default_value;
    }

    _settings.push_back({.declaration = declaration, .value = value});
    return true;
  }

  bool SettingsStore::Has(const std::string &name) const
  {
    return FindSetting(name) != nullptr;
  }

  const SettingDeclaration *SettingsStore::Find(const std::string &name) const
  {
    const Setting *setting = FindSetting(name);
    return setting != nullptr ? &setting->declaration : nullptr;
  }

  const DataValue *SettingsStore::Get(const std::string &name) const
  {
    const Setting *setting = FindSetting(name);
    return setting != nullptr ? &setting->value : nullptr;
  }

  bool SettingsStore::GetFlag(const std::string &name, bool &flag) const
  {
    const DataValue *value = Get(name);
    return value != nullptr && value->GetBool(flag);
  }

  bool SettingsStore::GetNumber(const std::string &name, double &number) const
  {
    const DataValue *value = Get(name);
    return value != nullptr && value->GetNumber(number);
  }

  bool SettingsStore::GetText(const std::string &name, std::string &text) const
  {
    const DataValue *value = Get(name);
    return value != nullptr && value->GetText(text);
  }

  void SettingsStore::Refuse(const std::string &name, const std::string &why)
  {
    if (!_refused.insert(name).second) { return; }
    _logger->Warn("The setting {} cannot be set: {}", name, why);
  }

  bool SettingsStore::Set(const std::string &name, const DataValue &value)
  {
    Setting *setting = FindSetting(name);
    if (setting == nullptr)
    {
      Refuse(name, std::format("it is not declared. The settings are: {}", list_of(Names())));
      return false;
    }

    if (setting->declaration.kind == SettingKind::Action)
    {
      Refuse(name, "it is an action, which is triggered rather than set");
      return false;
    }

    if (std::string why; !SettingsStore::Fits(setting->declaration, value, why))
    {
      Refuse(name, why);
      return false;
    }

    if (same(setting->value, value)) { return true; }

    // a number is stored as a number, whatever precision it was written with
    setting->value = value;
    Notify(name, setting->value);
    return true;
  }

  bool SettingsStore::Trigger(const std::string &name)
  {
    const Setting *setting = FindSetting(name);
    if (setting == nullptr)
    {
      Refuse(name, std::format("it is not declared. The settings are: {}", list_of(Names())));
      return false;
    }
    if (setting->declaration.kind != SettingKind::Action)
    {
      Refuse(name, std::format("it is {}, not an action, and is set rather than triggered", setting_kind_name(setting->declaration.kind)));
      return false;
    }

    Notify(name, setting->value);
    return true;
  }

  std::vector<std::string> SettingsStore::Names() const
  {
    std::vector<std::string> names;
    names.reserve(_settings.size());
    for (const auto &setting : _settings) { names.push_back(setting.declaration.name); }
    return names;
  }

  void SettingsStore::Notify(const std::string &name, const DataValue &value)
  {
    _notifying++;

    // by index, since a callback may subscribe meanwhile, which is not
    // called for this change; one that unsubscribes leaves a hole that is
    // swept below
    const std::size_t count = _subscribers.size();
    for (std::size_t i = 0; i < count; i++)
    {
      if (_subscribers[i].name != name || !_subscribers[i].callback) { continue; }
      _subscribers[i].callback(name, value);
    }

    _notifying--;
    if (_notifying == 0 && _ended_while_notifying)
    {
      _ended_while_notifying = false;
      std::erase_if(_subscribers, [](const Subscriber &subscriber) { return !subscriber.callback; });
    }
  }

  SettingsSubscription SettingsStore::OnChange(const std::string &name, const SettingsCallback &callback)
  {
    if (!Has(name))
    {
      const std::string names = list_of(Names());
      _logger->Warn("Nothing is heard of the setting {}, which is not declared. The settings are: {}", name, names);
      return {};
    }

    const int id = _next_subscriber++;
    _subscribers.push_back({.id = id, .name = name, .callback = callback});
    return {this, _token, id};
  }

  void SettingsStore::Unsubscribe(const int id)
  {
    const auto it = std::ranges::find(_subscribers, id, &Subscriber::id);
    if (it == _subscribers.end()) { return; }

    if (_notifying > 0)
    {
      // inside a notification: the hole is swept once it is done
      it->callback = nullptr;
      _ended_while_notifying = true;
      return;
    }
    _subscribers.erase(it);
  }
} // neon
