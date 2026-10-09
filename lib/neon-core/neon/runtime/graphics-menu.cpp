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
  }

  GraphicsMenu::GraphicsMenu(
    UiContext *ui,
    RenderContext *render,
    WindowContext *window,
    PlayerSettings *player,
    const std::shared_ptr<Logger> &logger)
  {
    _ui = ui;
    _render = render;
    _window = window;
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
      .value = "anisotropy", .section = "rendering", .name = "anisotropy",
      .current = [this] { return std::format("{}", _render->GetAnisotropy()); },
      .apply = [this](const std::string &text)
      {
        int level = 0;
        return ReadWhole(text, level) && _render->SetAnisotropy(level);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "texture_scale", .section = "rendering", .name = "texture_scale",
      .current = [this] { return Write(_render->GetTextureScale()); },
      .apply = [this](const std::string &text)
      {
        double scale = 0.0;
        return ReadNumber(text, scale) && _render->SetTextureScale(scale);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "target_scale", .section = "rendering", .name = "target_scale",
      .current = [this] { return Write(_render->GetTargetScale()); },
      .apply = [this](const std::string &text)
      {
        double scale = 0.0;
        return ReadNumber(text, scale) && _render->SetTargetScale(scale);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "target_mipmaps", .section = "rendering", .name = "target_mipmaps",
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
      .value = "shadow_map_size", .section = "rendering", .name = "shadow_map_size",
      .current = [this] { return std::format("{}", _render->GetShadowMapSize()); },
      .apply = [this](const std::string &text)
      {
        int size = 0;
        return ReadWhole(text, size) && _render->SetShadowMapSize(size);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "shadow_filter", .section = "rendering", .name = "shadow_filter",
      .current = [this] { return std::string(shadow_filters[static_cast<std::size_t>(_render->GetShadowFilter())]); },
      .apply = [this](const std::string &text)
      {
        const int place = PlaceOf(shadow_filters, text);
        return place >= 0 && _render->SetShadowFilter(static_cast<ShadowFilter>(place));
      },
      .kept = AsText,
    });

    _settings.push_back({
      .value = "shadow_cascades", .section = "rendering", .name = "shadow_cascades",
      .current = [this] { return std::format("{}", _render->GetShadowCascades()); },
      .apply = [this](const std::string &text)
      {
        int cascades = 0;
        return ReadWhole(text, cascades) && _render->SetShadows(_render->GetShadowDistance(), cascades);
      },
      .kept = AsNumber,
    });

    _settings.push_back({
      .value = "shadow_distance", .section = "rendering", .name = "shadow_distance",
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
    for (auto &setting : _settings)
    {
      setting.opened = setting.current();
      setting.applied = setting.opened;
      setting.refused.clear();

      // a flag as a flag and a number as a number, so that a box that is
      // ticked and a slider follow it; a choice as the text it is
      double number = 0.0;
      if (setting.opened == "true" || setting.opened == "false")
      {
        _ui->SetFlag(setting.value, setting.opened == "true");
      } else if (setting.value == "shadow_distance" || setting.value == "exposure")
      {
        (void) ReadNumber(setting.opened, number);
        _ui->SetNumber(setting.value, number);
      } else
      {
        _ui->SetText(setting.value, setting.opened);
      }
    }
    _is_open = true;
  }

  void GraphicsMenu::Update()
  {
    if (!_is_open) { return; }

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
  }

  void GraphicsMenu::Close(const bool keep)
  {
    if (!_is_open) { return; }

    Update();
    _is_open = false;

    for (auto &setting : _settings)
    {
      if (setting.applied == setting.opened) { continue; }

      if (keep)
      {
        if (_player != nullptr) { _player->Set(setting.section, setting.name, setting.kept(setting.applied)); }
      } else if (!setting.apply(setting.opened))
      {
        _logger->Warn("The menu could not put {} back to {}", setting.value, setting.opened);
      }
    }

    if (keep && _player != nullptr) { (void) _player->Write(); }
  }

  std::vector<std::string> GraphicsMenu::GetValueNames() const
  {
    std::vector<std::string> names;
    for (const auto &setting : _settings) { names.push_back(setting.value); }
    return names;
  }
} // neon
