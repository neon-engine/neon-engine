#include "graphics-menu.hpp"

#include <format>
#include <map>
#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-render-context.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/mock-window-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::DataValue;
  using neon::DocumentFormat;
  using neon::GraphicsMenu;
  using neon::PlayerSettings;
  using neon::Tonemapper;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::MockRenderContext;
  using neon::testing::MockUiContext;
  using neon::testing::MockWindowContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::NiceMock;

  // A format that reads nothing and writes a line, for a test that looks at
  // what would be written rather than at the text.
  class WrittenFormat final : public DocumentFormat
  {
  public:
    bool Read(const std::string &, const std::string &, DataValue &, std::string &error) override
    {
      error = "not read in this test";
      return false;
    }

    std::string Write(const DataValue &) override { return "written\n"; }
  };

  class GraphicsMenuTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    NiceMock<MockUiContext> _ui;
    NiceMock<MockRenderContext> _render{_logger};
    NiceMock<MockWindowContext> _window;
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    WrittenFormat _format;
    PlayerSettings _player{&_files, &_format, _logger};

    // what the user interface holds, as text, as the menu's elements would
    std::map<std::string, std::string> _values;

    // the graphics as they are
    bool _vsync = true;
    int _max_fps = 0;
    WindowMode _mode = WindowMode::Borderless;
    int _anisotropy = 8;
    double _texture_scale = 1.0;
    double _target_scale = 1.0;
    int _target_mipmaps = 0;
    double _shadow_distance = 120.0;
    int _shadow_cascades = 4;
    Tonemapper _tonemapper = Tonemapper::None;
    double _exposure = 1.0;

    void SetUp() override
    {
      _files.Initialize();

      ON_CALL(_ui, SetText(_, _)).WillByDefault([this](const std::string &name, const std::string &text)
      {
        _values[name] = text;
      });
      ON_CALL(_ui, SetFlag(_, _)).WillByDefault([this](const std::string &name, const bool flag)
      {
        _values[name] = flag ? "true" : "false";
      });
      ON_CALL(_ui, SetNumber(_, _)).WillByDefault([this](const std::string &name, const double number)
      {
        _values[name] = std::format("{}", number);
      });
      ON_CALL(_ui, GetValue(_, _)).WillByDefault([this](const std::string &name, bool *is_set)
      {
        const auto found = _values.find(name);
        if (is_set != nullptr) { *is_set = found != _values.end(); }
        return found != _values.end() ? found->second : std::string();
      });

      ON_CALL(_render, GetVerticalSync()).WillByDefault([this] { return _vsync; });
      ON_CALL(_render, SetVerticalSync(_)).WillByDefault([this](const bool on) { _vsync = on; return true; });
      ON_CALL(_render, GetAnisotropy()).WillByDefault([this] { return _anisotropy; });
      ON_CALL(_render, SetAnisotropy(_)).WillByDefault([this](const int level)
      {
        if (!neon::Anisotropy::IsLevel(level)) { return false; }
        _anisotropy = level;
        return true;
      });
      ON_CALL(_render, GetTextureScale()).WillByDefault([this] { return _texture_scale; });
      ON_CALL(_render, SetTextureScale(_)).WillByDefault([this](const double scale) { _texture_scale = scale; return true; });
      ON_CALL(_render, GetTargetScale()).WillByDefault([this] { return _target_scale; });
      ON_CALL(_render, SetTargetScale(_)).WillByDefault([this](const double scale) { _target_scale = scale; return true; });
      ON_CALL(_render, GetTargetMipmaps()).WillByDefault([this] { return _target_mipmaps; });
      ON_CALL(_render, SetTargetMipmaps(_)).WillByDefault([this](const int mipmaps) { _target_mipmaps = mipmaps; return true; });
      ON_CALL(_render, GetShadowDistance()).WillByDefault([this] { return _shadow_distance; });
      ON_CALL(_render, GetShadowCascades()).WillByDefault([this] { return _shadow_cascades; });
      ON_CALL(_render, SetShadows(_, _)).WillByDefault([this](const double distance, const int cascades)
      {
        _shadow_distance = distance;
        _shadow_cascades = cascades;
        return true;
      });
      ON_CALL(_render, GetTonemapper()).WillByDefault([this] { return _tonemapper; });
      ON_CALL(_render, GetExposure()).WillByDefault([this] { return _exposure; });
      ON_CALL(_render, SetTonemapping(_, _)).WillByDefault([this](const Tonemapper tonemapper, const double exposure)
      {
        _tonemapper = tonemapper;
        _exposure = exposure;
        return true;
      });

      ON_CALL(_window, GetWindowMode()).WillByDefault([this] { return _mode; });
      ON_CALL(_window, SetWindowMode(_)).WillByDefault([this](const WindowMode mode) { _mode = mode; return true; });
      ON_CALL(_window, GetFrameLimit()).WillByDefault([this] { return _max_fps; });
      ON_CALL(_window, SetFrameLimit(_)).WillByDefault([this](const int most) { _max_fps = most; });
    }

    GraphicsMenu Menu(PlayerSettings *player = nullptr)
    {
      return GraphicsMenu(&_ui, &_render, &_window, player, _logger);
    }

    /// What the file of the player would hold under a name, or null.
    const DataValue *Kept(const std::string &section, const std::string &name)
    {
      const DataValue *found = _player.Document().Find(section);
      return found != nullptr ? found->Find(name) : nullptr;
    }
  };

  TEST_F(GraphicsMenuTest, ShowsTheGraphicsAsTheyAreWhenItIsOpened)
  {
    GraphicsMenu menu = Menu();
    menu.Open();

    EXPECT_EQ(_values["window_mode"], "borderless");
    EXPECT_EQ(_values["vsync"], "true");
    EXPECT_EQ(_values["max_fps"], "0");
    EXPECT_EQ(_values["anisotropy"], "8");
    EXPECT_EQ(_values["texture_scale"], "1");
    EXPECT_EQ(_values["target_scale"], "1");
    EXPECT_EQ(_values["target_mipmaps"], "0");
    EXPECT_EQ(_values["shadow_cascades"], "4");
    EXPECT_EQ(_values["shadow_distance"], "120");
    EXPECT_EQ(_values["tonemapper"], "none");
    EXPECT_EQ(_values["exposure"], "1");
  }

  TEST_F(GraphicsMenuTest, HasAValueForEveryGraphicsSetting)
  {
    EXPECT_EQ(Menu().GetValueNames().size(), 11u);
  }

  TEST_F(GraphicsMenuTest, ChangesWhatThePlayerChangesAtOnce)
  {
    GraphicsMenu menu = Menu();
    menu.Open();

    _values["anisotropy"] = "16";
    _values["vsync"] = "false";
    _values["window_mode"] = "windowed";
    _values["max_fps"] = "144";
    _values["shadow_cascades"] = "2";
    _values["tonemapper"] = "agx";
    _values["exposure"] = "1.5";
    _values["target_mipmaps"] = "4";
    menu.Update();

    EXPECT_EQ(_anisotropy, 16);
    EXPECT_FALSE(_vsync);
    EXPECT_EQ(_mode, WindowMode::Windowed);
    EXPECT_EQ(_max_fps, 144);
    EXPECT_EQ(_shadow_cascades, 2);
    EXPECT_DOUBLE_EQ(_shadow_distance, 120.0) << "and the distance stays";
    EXPECT_EQ(_tonemapper, Tonemapper::Agx);
    EXPECT_DOUBLE_EQ(_exposure, 1.5);
    EXPECT_EQ(_target_mipmaps, 4);
  }

  TEST_F(GraphicsMenuTest, ChangesNothingWhenNothingChanged)
  {
    GraphicsMenu menu = Menu();
    menu.Open();

    EXPECT_CALL(_render, SetAnisotropy(_)).Times(0);
    EXPECT_CALL(_render, SetVerticalSync(_)).Times(0);
    EXPECT_CALL(_window, SetWindowMode(_)).Times(0);
    menu.Update();
    menu.Update();
  }

  TEST_F(GraphicsMenuTest, KeepsWhatChangedWhenTheMenuIsClosedWithApply)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["anisotropy"] = "4";
    _values["vsync"] = "false";
    menu.Update();

    menu.Close(true);

    ASSERT_NE(Kept("rendering", "anisotropy"), nullptr);
    ASSERT_NE(Kept("rendering", "vsync"), nullptr);
    EXPECT_EQ(Kept("rendering", "exposure"), nullptr) << "what the player never changed stays the project's";
    EXPECT_EQ(Kept("window", "mode"), nullptr);
    EXPECT_TRUE(_files.Exists("user://settings.yml"));
  }

  TEST_F(GraphicsMenuTest, TakesWhatChangedInTheFrameTheMenuIsClosed)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["anisotropy"] = "2";

    menu.Close(true);

    EXPECT_EQ(_anisotropy, 2);
    EXPECT_NE(Kept("rendering", "anisotropy"), nullptr);
  }

  TEST_F(GraphicsMenuTest, PutsBackWhatWasNotKept)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["anisotropy"] = "16";
    _values["window_mode"] = "windowed";
    menu.Update();

    menu.Close(false);

    EXPECT_EQ(_anisotropy, 8);
    EXPECT_EQ(_mode, WindowMode::Borderless);
    EXPECT_FALSE(_files.Exists("user://settings.yml"));
  }

  TEST_F(GraphicsMenuTest, SaysOnceWhatCannotBeTakenAndKeepsNothingOfIt)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["anisotropy"] = "3";
    menu.Update();
    menu.Update();
    menu.Update();

    EXPECT_EQ(_anisotropy, 8);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u) << _logger->Messages(LogLevel::Warn);

    menu.Close(true);
    EXPECT_EQ(Kept("rendering", "anisotropy"), nullptr);
  }

  TEST_F(GraphicsMenuTest, TakesTheNumberOfASliderAsTheSameNumber)
  {
    GraphicsMenu menu = Menu();
    menu.Open();

    // a slider may say 120 as 120.0, which is no change
    _values["shadow_distance"] = "120.0";
    EXPECT_CALL(_render, SetShadows(_, _)).Times(0);
    menu.Update();
  }

  TEST_F(GraphicsMenuTest, DoesNothingUntilItIsOpened)
  {
    GraphicsMenu menu = Menu();
    _values["anisotropy"] = "16";

    menu.Update();

    EXPECT_EQ(_anisotropy, 8);
    EXPECT_FALSE(menu.IsOpen());
  }
} // namespace
