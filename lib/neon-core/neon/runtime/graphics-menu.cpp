#include "graphics-menu.hpp"

#include <array>
#include <cstdlib>
#include <format>

#include <neon/render/anisotropy.hpp>
#include <neon/render/shadow-filter.hpp>
#include <neon/render/target-quality.hpp>
#include <neon/render/texture-scale.hpp>
#include <neon/render/tonemapper.hpp>
#include <neon/window/frame-limit.hpp>
#include <neon/window/window-mode.hpp>

namespace neon
{
  // Helpers of GraphicsMenu: the texts of the menu, and the numbers and
  // names they stand for.
  namespace
  {
    /// The number a text holds, all of it, or false.
    bool ReadNumber(const std::string &text, double &number)
    {
      if (text.empty()) { return false; }

      char *end = nullptr;
      number = std::strtod(text.c_str(), &end);
      return end == text.c_str() + text.size();
    }

    /// A whole number a text holds, or false.
    bool ReadWhole(const std::string &text, int &whole)
    {
      double number = 0.0;
      if (!ReadNumber(text, number) || number != static_cast<double>(static_cast<int>(number))) { return false; }

      whole = static_cast<int>(number);
      return true;
    }

    std::string Write(const double number)
    {
      return std::format("{}", number);
    }

    constexpr std::array<const char *, 3> window_modes = {"windowed", "borderless", "fullscreen"};
    constexpr std::array<const char *, 3> tonemappers = {"none", "aces", "agx"};
    constexpr std::array<const char *, 2> shadow_filters = {"none", "pcf"};

    /// The place of a name in a list of names, or -1.
    template<std::size_t Count>
    int PlaceOf(const std::array<const char *, Count> &names, const std::string &name)
    {
      for (std::size_t i = 0; i < Count; i++)
      {
        if (name == names[i]) { return static_cast<int>(i); }
      }
      return -1;
    }

    DataValue AsNumber(const std::string &text)
    {
      double number = 0.0;
      (void) ReadNumber(text, number);
      return DataValue::Number(number);
    }

    DataValue AsText(const std::string &text)
    {
      return DataValue::Text(text);
    }

    const std::string quality = "quality";
    const std::string rendering = "rendering";
  }

  GraphicsMenu::GraphicsMenu(
    UiContext *ui,
    RenderContext *render,
    WindowContext *window,
    const GraphicsPresets &presets,
    PlayerSettings *player,
    const std::shared_ptr<Logger> &logger)
  {
    _ui = ui;
    _render = render;
    _window = window;
    _presets = presets;
    _player = player;
    _logger = logger;
    Describe();
  }

  void GraphicsMenu::Describe()
  {
    _settings.push_back({
      .value = "window_mode", .section = "window", .name = "mode",
      .current = [this] { return std::string(window_modes[static_cast<std::size_t>(_window->GetWindowMode())]); },
      .apply = [this](const std::string &text)
      {
        const int place = PlaceOf(window_modes, text);
        return place >= 0 && _window->SetWindowMode(static_cast<WindowMode>(place));
      },
      .kept = AsText,
    });

    _settings.push_back({
      .value = "vsync", .section = "rendering", .name = "vsync",
      .current = [this] { return std::string(_render->GetVerticalSync() ? "true" : "false"); },
      .apply = [this](const std::string &text)
      {
        return (text == "true" || text == "false") && _render->SetVerticalSync(text == "true");
      },
      .kept = [](const std::string &text) { return DataValue::Bool(text == "true"); },
    });

    _settings.push_back({
      .value = "max_fps", .section = "rendering", .name = "max_fps",
      .current = [this] { return std::format("{}", _window->GetFrameLimit()); },
      .apply = [this](const std::string &text)
      {
        int most = 0;
        if (!ReadWhole(text, most) || (most != 0 && (most < FrameLimit::kLeast || most > FrameLimit::kMost)))
        {
          return false;
        }
        _window->SetFrameLimit(most);
        return true;
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "anisotropy", .section = "rendering", .name = "anisotropy", .decided = true,
      .current = [this] { return std::format("{}", _render->GetAnisotropy()); },
      .apply = [this](const std::string &text)
      {
        int level = 0;
        return ReadWhole(text, level) && _render->SetAnisotropy(level);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "texture_scale", .section = "rendering", .name = "texture_scale", .decided = true,
      .current = [this] { return Write(_render->GetTextureScale()); },
      .apply = [this](const std::string &text)
      {
        double scale = 0.0;
        return ReadNumber(text, scale) && _render->SetTextureScale(scale);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "target_scale", .section = "rendering", .name = "target_scale", .decided = true,
      .current = [this] { return Write(_render->GetTargetScale()); },
      .apply = [this](const std::string &text)
      {
        double scale = 0.0;
        return ReadNumber(text, scale) && _render->SetTargetScale(scale);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "target_mipmaps", .section = "rendering", .name = "target_mipmaps", .decided = true,
      .current = [this] { return std::format("{}", _render->GetTargetMipmaps()); },
      .apply = [this](const std::string &text)
      {
        int mipmaps = 0;
        return ReadWhole(text, mipmaps) && _render->SetTargetMipmaps(mipmaps);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "shadows", .section = "rendering", .name = "shadows",
      .current = [this] { return std::string(_render->GetShadowsEnabled() ? "true" : "false"); },
      .apply = [this](const std::string &text)
      {
        return (text == "true" || text == "false") && _render->SetShadowsEnabled(text == "true");
      },
      .kept = [](const std::string &text) { return DataValue::Bool(text == "true"); },
    });

    _settings.push_back({
      .value = "shadow_map_size", .section = "rendering", .name = "shadow_map_size", .decided = true,
      .current = [this] { return std::format("{}", _render->GetShadowMapSize()); },
      .apply = [this](const std::string &text)
      {
        int size = 0;
        return ReadWhole(text, size) && _render->SetShadowMapSize(size);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "shadow_filter", .section = "rendering", .name = "shadow_filter", .decided = true,
      .current = [this] { return std::string(shadow_filters[static_cast<std::size_t>(_render->GetShadowFilter())]); },
      .apply = [this](const std::string &text)
      {
        const int place = PlaceOf(shadow_filters, text);
        return place >= 0 && _render->SetShadowFilter(static_cast<ShadowFilter>(place));
      },
      .kept = AsText,
    });

    _settings.push_back({
      .value = "shadow_cascades", .section = "rendering", .name = "shadow_cascades", .decided = true,
      .current = [this] { return std::format("{}", _render->GetShadowCascades()); },
      .apply = [this](const std::string &text)
      {
        int cascades = 0;
        return ReadWhole(text, cascades) && _render->SetShadows(_render->GetShadowDistance(), cascades);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "shadow_distance", .section = "rendering", .name = "shadow_distance", .decided = true,
      .current = [this] { return Write(_render->GetShadowDistance()); },
      .apply = [this](const std::string &text)
      {
        double distance = 0.0;
        return ReadNumber(text, distance) && _render->SetShadows(distance, _render->GetShadowCascades());
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "tonemapper", .section = "rendering", .name = "tonemapper",
      .current = [this] { return std::string(tonemappers[static_cast<std::size_t>(_render->GetTonemapper())]); },
      .apply = [this](const std::string &text)
      {
        const int place = PlaceOf(tonemappers, text);
        return place >= 0 && _render->SetTonemapping(static_cast<Tonemapper>(place), _render->GetExposure());
      },
      .kept = AsText,
    });

    _settings.push_back({
      .value = "exposure", .section = "rendering", .name = "exposure",
      .current = [this] { return Write(_render->GetExposure()); },
      .apply = [this](const std::string &text)
      {
        double exposure = 0.0;
        return ReadNumber(text, exposure) && _render->SetTonemapping(_render->GetTonemapper(), exposure);
      },
      .kept = AsNumber,
    });
  }

  void GraphicsMenu::Open()
  {
    // the Quality row offers the presets of the project, which its file
    // cannot know; a menu without such a row is left as it is
    if (const UiHandle select = _ui->FindByName(quality); select.IsSet())
    {
      const std::vector<std::string> names = _presets.Names();
      if (_ui->SetField(select, "options", names))
      {
        std::string offered;
        for (const auto &name : names) { offered += (offered.empty() ? "" : ", ") + name; }
        _logger->Info("The menu offers the presets {}", offered);
      } else
      {
        _logger->Warn("The options of {} of the menu cannot be set to the presets", quality);
      }
    }

    for (auto &setting : _settings)
    {
      setting.opened = setting.current();
      setting.applied = setting.opened;
      setting.refused.clear();
      Show(setting, setting.opened);
    }

    _quality_opened = QualityNow();
    _quality_shown = _quality_opened;
    _quality_refused.clear();
    _ui->SetText(quality, _quality_shown);
    _is_open = true;
  }

  void GraphicsMenu::Show(const Setting &setting, const std::string &text) const
  {
    // a flag as a flag and a number as a number, so that a box that is
    // ticked and a slider follow it; a choice as the text it is
    if (text == "true" || text == "false")
    {
      _ui->SetFlag(setting.value, text == "true");
    } else if (setting.value == "shadow_distance" || setting.value == "exposure")
    {
      double number = 0.0;
      (void) ReadNumber(text, number);
      _ui->SetNumber(setting.value, number);
    } else
    {
      _ui->SetText(setting.value, text);
    }
  }

  std::string GraphicsMenu::QualityNow() const
  {
    const GraphicsPreset values{
      .name = std::string(GraphicsPresets::kCustom),
      .anisotropy = _render->GetAnisotropy(),
      .texture_scale = _render->GetTextureScale(),
      .target_scale = _render->GetTargetScale(),
      .target_mipmaps = _render->GetTargetMipmaps(),
      .shadow_map_size = _render->GetShadowMapSize(),
      .shadow_filter = _render->GetShadowFilter(),
      .shadow_cascades = _render->GetShadowCascades(),
      .shadow_distance = _render->GetShadowDistance(),
    };
    return std::string(_presets.NameOf(values));
  }

  void GraphicsMenu::UpdateQuality()
  {
    bool is_set = false;
    const std::string chosen = _ui->GetValue(quality, &is_set);
    if (!is_set || chosen == _quality_shown || chosen == _quality_refused) { return; }

    const GraphicsPreset *preset = _presets.Named(chosen);
    if (preset == nullptr && chosen != GraphicsPresets::kCustom)
    {
      _logger->Warn("The menu asked for {} of {}, which is no preset of the graphics", chosen, quality);
      _quality_refused = chosen;
      return;
    }

    _logger->Info("The menu set {} to {}", quality, chosen);
    _quality_shown = chosen;
    if (preset == nullptr) { return; }

    // the values the preset decides are shown as if the player had chosen
    // each, and take effect below as every value does
    for (const auto &setting : _settings)
    {
      if (setting.value == "anisotropy") { Show(setting, std::format("{}", preset->anisotropy)); }
      if (setting.value == "texture_scale") { Show(setting, Write(preset->texture_scale)); }
      if (setting.value == "target_scale") { Show(setting, Write(preset->target_scale)); }
      if (setting.value == "target_mipmaps") { Show(setting, std::format("{}", preset->target_mipmaps)); }
      if (setting.value == "shadow_map_size") { Show(setting, std::format("{}", preset->shadow_map_size)); }
      if (setting.value == "shadow_filter")
      {
        Show(setting, std::string(shadow_filters[static_cast<std::size_t>(preset->shadow_filter)]));
      }
      if (setting.value == "shadow_cascades") { Show(setting, std::format("{}", preset->shadow_cascades)); }
      if (setting.value == "shadow_distance") { Show(setting, Write(preset->shadow_distance)); }
    }
  }

  void GraphicsMenu::Update()
  {
    if (!_is_open) { return; }

    UpdateQuality();

    for (auto &setting : _settings)
    {
      bool is_set = false;
      std::string shown = _ui->GetValue(setting.value, &is_set);
      if (!is_set) { continue; }

      // a slider says what it holds as a number, which is compared as one
      if (double number = 0.0; ReadNumber(shown, number)) { shown = Write(number); }
      if (shown == setting.applied || shown == setting.refused) { continue; }

      // said once, and not in every frame the menu goes on showing it
      if (!setting.apply(shown))
      {
        _logger->Warn("The menu asked for {} of {}, which cannot be taken", shown, setting.value);
        setting.refused = shown;
        continue;
      }

      _logger->Info("The menu set {} to {}", setting.value, shown);
      setting.applied = shown;
    }

    // a value changed by hand makes the preset custom, and values that
    // happen to match one show its name
    if (const std::string now = QualityNow(); now != _quality_shown)
    {
      _quality_shown = now;
      _ui->SetText(quality, now);
    }
  }

  void GraphicsMenu::Close(const bool keep)
  {
    if (!_is_open) { return; }

    Update();
    _is_open = false;

    // Whether a preset holds: the file then names it alone, since a value
    // it decides written next to it would be left out with a warning. With
    // custom, the values that changed are written, and custom with them, so
    // that they apply over a preset a layer before chose.
    const bool is_preset = _quality_shown != GraphicsPresets::kCustom;
    bool decided_changed = false;

    for (auto &setting : _settings)
    {
      if (setting.applied == setting.opened) { continue; }
      if (setting.decided) { decided_changed = true; }

      if (keep)
      {
        if (_player == nullptr) { continue; }
        if (setting.decided && is_preset) { _player->Remove(setting.section, setting.name); }
        else { _player->Set(setting.section, setting.name, setting.kept(setting.applied)); }
      } else if (!setting.apply(setting.opened))
      {
        _logger->Warn("The menu could not put {} back to {}", setting.value, setting.opened);
      }
    }

    if (keep && _player != nullptr && (_quality_shown != _quality_opened || decided_changed))
    {
      _player->Set(rendering, quality, DataValue::Text(_quality_shown));
      if (is_preset)
      {
        for (const auto &setting : _settings)
        {
          if (setting.decided) { _player->Remove(setting.section, setting.name); }
        }
      }
    }

    if (keep && _player != nullptr) { (void) _player->Write(); }
  }

  std::vector<std::string> GraphicsMenu::GetValueNames() const
  {
    std::vector<std::string> names{quality};
    for (const auto &setting : _settings) { names.push_back(setting.value); }
    return names;
  }
} // neon
