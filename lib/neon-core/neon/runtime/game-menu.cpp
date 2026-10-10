#include "game-menu.hpp"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <map>

namespace neon
{
  // Helpers of GameMenu: the texts of the menu and the values they stand for.
  namespace
  {
    /// The number a text holds, all of it, or false.
    bool read_number(const std::string &text, double &number)
    {
      if (text.empty()) { return false; }

      char *end = nullptr;
      number = std::strtod(text.c_str(), &end);
      return end == text.c_str() + text.size();
    }

    bool same(const DataValue &a, const DataValue &b)
    {
      if (a.GetKind() != b.GetKind()) { return false; }
      if (a.IsEmpty()) { return true; }

      bool x = false;
      bool y = false;
      if (a.GetBool(x) && b.GetBool(y)) { return x == y; }

      double m = 0.0;
      double n = 0.0;
      if (a.GetNumber(m) && b.GetNumber(n)) { return m == n; }

      std::string s;
      std::string t;
      return a.GetText(s) && b.GetText(t) && s == t;
    }

    /// A value as the log shows it.
    std::string written(const DataValue &value)
    {
      bool flag = false;
      double number = 0.0;
      std::string text;
      if (value.GetBool(flag)) { return flag ? "true" : "false"; }
      if (value.GetNumber(number)) { return std::format("{}", number); }
      (void) value.GetText(text);
      return text;
    }
  }

  GameMenu::GameMenu(UiContext *ui, SettingsStore *store, PlayerSettings *player, const std::shared_ptr<Logger> &logger)
  {
    _ui = ui;
    _store = store;
    _player = player;
    _logger = logger;
  }

  void GameMenu::Show(const std::string &name, const DataValue &value) const
  {
    bool flag = false;
    double number = 0.0;
    std::string text;
    if (value.GetBool(flag)) { _ui->SetFlag(name, flag); }
    else if (value.GetNumber(number)) { _ui->SetNumber(name, number); }
    else if (value.GetText(text)) { _ui->SetText(name, text); }
  }

  bool GameMenu::Read(const Row &row, const std::string &shown, DataValue &value) const
  {
    const SettingDeclaration *declaration = _store->Find(row.name);
    if (declaration == nullptr) { return false; }

    switch (declaration->kind)
    {
      case SettingKind::Flag:
        if (shown != "true" && shown != "false") { return false; }
        value = DataValue::Bool(shown == "true");
        return true;
      case SettingKind::Number:
      {
        double number = 0.0;
        if (!read_number(shown, number)) { return false; }
        value = DataValue::Number(number);
        return true;
      }
      case SettingKind::Text:
      case SettingKind::Choice:
        value = DataValue::Text(shown);
        return true;
      case SettingKind::Action:
        return false;
    }
    return false;
  }

  DataValue GameMenu::DescribeRow(const SettingDeclaration &declaration)
  {
    const std::string &name = declaration.name;
    const std::string bound = "{" + name + "}";

    DataValue control = DataValue::Map();
    switch (declaration.GetControl())
    {
      case SettingControl::Slider:
      {
        DataValue slider = DataValue::Map();
        slider.Set("type", DataValue::Text("slider"));
        slider.Set("name", DataValue::Text(name));
        slider.Set("min", DataValue::Number(declaration.least.value_or(0.0)));
        slider.Set("max", DataValue::Number(declaration.most.value_or(1.0)));
        if (declaration.step.has_value()) { slider.Set("step", DataValue::Number(*declaration.step)); }
        slider.Set("value", DataValue::Text(bound));

        DataValue shown = DataValue::Map();
        shown.Set("type", DataValue::Text("label"));
        shown.Set("class", DataValue::Text("value"));
        shown.Set("text", DataValue::Text(bound));

        control.Set("type", DataValue::Text("panel"));
        control.Set("class", DataValue::Text("with-value"));
        DataValue &children = control.Set("children", DataValue::List());
        children.Add(slider);
        children.Add(shown);
        break;
      }
      case SettingControl::Toggle:
      case SettingControl::Checkbox:
        control.Set("type", DataValue::Text(declaration.GetControl() == SettingControl::Toggle ? "toggle" : "checkbox"));
        control.Set("name", DataValue::Text(name));
        control.Set("checked", DataValue::Text(bound));
        break;
      case SettingControl::Dropdown:
      {
        control.Set("type", DataValue::Text("select"));
        control.Set("name", DataValue::Text(name));
        control.Set("value", DataValue::Text(bound));
        DataValue &options = control.Set("options", DataValue::List());
        for (const std::string &choice : declaration.choices)
        {
          DataValue option = DataValue::Map();
          option.Set("value", DataValue::Text(choice));
          option.Set("text", DataValue::Text(choice));
          options.Add(option);
        }
        break;
      }
      case SettingControl::Field:
        control.Set("type", DataValue::Text("input"));
        control.Set("name", DataValue::Text(name));
        control.Set("value", DataValue::Text(bound));
        if (declaration.kind == SettingKind::Number) { control.Set("kind", DataValue::Text("number")); }
        break;
      case SettingControl::Button:
        control.Set("type", DataValue::Text("button"));
        control.Set("name", DataValue::Text(name));
        control.Set("text", DataValue::Text(declaration.GetLabel()));
        break;
    }

    DataValue row = DataValue::Map();
    row.Set("type", DataValue::Text("panel"));
    row.Set("class", DataValue::Text("row"));
    row.Set("name", DataValue::Text(name + "-row"));
    DataValue &children = row.Set("children", DataValue::List());

    // a button carries its own label
    if (declaration.GetControl() != SettingControl::Button)
    {
      DataValue label = DataValue::Map();
      label.Set("type", DataValue::Text("label"));
      label.Set("class", DataValue::Text("row-label"));
      label.Set("text", DataValue::Text(declaration.GetLabel()));
      children.Add(label);
    }
    children.Add(control);
    return row;
  }

  void GameMenu::Build(const int document) const
  {
    const UiHandle root = _ui->GetRoot(document);
    const UiHandle list = _ui->FindByName("list", root);
    if (!list.IsSet()) { return; }

    // the settings no element of the document is named after, by category
    // and group, each in the order of the first setting that names it
    struct Group
    {
      std::string name;
      double order = 0.0;
      std::vector<const SettingDeclaration *> rows;
    };
    struct Category
    {
      std::string name;
      double order = 0.0;
      std::vector<Group> groups;
    };
    std::vector<Category> categories;

    for (const std::string &name : _store->Names())
    {
      const SettingDeclaration *declaration = _store->Find(name);
      if (declaration == nullptr || _ui->FindByName(name, root).IsSet()) { continue; }

      const std::string category = declaration->category.empty() ? "Game" : declaration->category;
      auto found = std::ranges::find(categories, category, &Category::name);
      if (found == categories.end())
      {
        categories.push_back({.name = category, .order = declaration->order});
        found = categories.end() - 1;
      }

      auto group = std::ranges::find(found->groups, declaration->group, &Group::name);
      if (group == found->groups.end())
      {
        found->groups.push_back({.name = declaration->group, .order = declaration->order});
        group = found->groups.end() - 1;
      }
      group->rows.push_back(declaration);
    }

    std::ranges::stable_sort(categories, {}, &Category::order);
    for (Category &category : categories)
    {
      // the rows under no heading come first, then the groups in order
      std::ranges::stable_sort(category.groups, [](const Group &a, const Group &b)
      {
        if (a.name.empty() != b.name.empty()) { return a.name.empty(); }
        return a.order < b.order;
      });
      for (Group &group : category.groups)
      {
        std::ranges::stable_sort(group.rows, {}, &SettingDeclaration::order);
      }
    }

    for (const Category &category : categories)
    {
      DataValue heading = DataValue::Map();
      heading.Set("type", DataValue::Text("label"));
      heading.Set("class", DataValue::Text("section"));
      heading.Set("text", DataValue::Text(category.name));
      (void) _ui->CreateFrom(heading, list);

      for (const Group &group : category.groups)
      {
        if (!group.name.empty())
        {
          DataValue subheading = DataValue::Map();
          subheading.Set("type", DataValue::Text("label"));
          subheading.Set("class", DataValue::Text("group"));
          subheading.Set("text", DataValue::Text(group.name));
          (void) _ui->CreateFrom(subheading, list);
        }
        for (const SettingDeclaration *declaration : group.rows) { (void) _ui->CreateFrom(DescribeRow(*declaration), list); }
      }
      _logger->Info("The settings menu shows {} as a section of its own", category.name);
    }
  }

  void GameMenu::Open(const int document)
  {
    if (document >= 0) { Build(document); }

    _rows.clear();
    for (const std::string &name : _store->Names())
    {
      const DataValue *value = _store->Get(name);
      if (value == nullptr) { continue; }

      _rows.push_back({.name = name, .opened = *value, .applied = *value});
      Show(name, *value);
    }
    _is_open = true;
  }

  void GameMenu::Update()
  {
    if (!_is_open) { return; }

    for (Row &row : _rows)
    {
      const SettingDeclaration *declaration = _store->Find(row.name);
      if (declaration != nullptr && declaration->kind == SettingKind::Action)
      {
        if (_ui->WasClicked(row.name) && _store->Trigger(row.name)) { _logger->Info("The menu pressed {}", row.name); }
        continue;
      }

      bool is_set = false;
      const std::string shown = _ui->GetValue(row.name, &is_set);
      if (!is_set || shown == row.refused) { continue; }

      DataValue value;
      if (!Read(row, shown, value))
      {
        // said once, and not in every frame the menu goes on showing it
        _logger->Warn("The menu asked for {} of {}, which cannot be taken", shown, row.name);
        row.refused = shown;
        continue;
      }
      if (same(value, row.applied)) { continue; }

      // the store says why, once for a name
      if (!_store->Set(row.name, value))
      {
        row.refused = shown;
        continue;
      }

      _logger->Info("The menu set {} to {}", row.name, shown);
      row.applied = value;
      row.refused.clear();
    }
  }

  void GameMenu::Close(const bool keep)
  {
    if (!_is_open) { return; }

    Update();
    _is_open = false;

    for (const Row &row : _rows)
    {
      if (same(row.applied, row.opened)) { continue; }

      if (keep)
      {
        if (_player != nullptr) { _player->Set("game", row.name, row.applied); }
      } else if (!_store->Set(row.name, row.opened))
      {
        const std::string opened = written(row.opened);
        _logger->Warn("The menu could not put {} back to {}", row.name, opened);
      }
    }

    if (keep && _player != nullptr) { (void) _player->Write(); }
  }

  std::vector<std::string> GameMenu::GetValueNames() const
  {
    return _store->Names();
  }
} // neon
