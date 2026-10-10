#include "graphics-menu.hpp"

#include <format>
#include <map>
#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/render/shadow-map-size.hpp>
#include <neon/testing/mock-render-context.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/mock-window-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::DataValue;
  using neon::DocumentFormat;
  using neon::FieldValue;
  using neon::GraphicsMenu;
  using neon::GraphicsPreset;
  using neon::GraphicsPresets;
  using neon::UiHandle;
  using neon::PlayerSettings;
  using neon::ShadowFilter;
  using neon::Tonemapper;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::MockRenderContext;
  using neon::testing::MockUiContext;
  using neon::testing::MockWindowContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::HasSubstr;
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

    // the select of the quality, and the options it was given last
    static constexpr UiHandle quality_select{.id = 7};
    std::vector<std::string> _quality_options;

    // the table the menu is made with, the engine's unless a test says
    GraphicsPresets _presets;

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
    bool _shadows = true;
    int _shadow_map_size = 2048;
    ShadowFilter _shadow_filter = ShadowFilter::Pcf;
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
      ON_CALL(_ui, FindByName(_, _)).WillByDefault([](const std::string &name, UiHandle)
      {
        return name == "quality" ? quality_select : UiHandle{};
      });
      ON_CALL(_ui, SetField(_, _, _)).WillByDefault([this](const UiHandle element, const std::string &name, const FieldValue &value)
      {
        if (element != quality_select || name != "options") { return false; }
        _quality_options = std::get<std::vector<std::string>>(value);
        return true;
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
      ON_CALL(_render, GetShadowsEnabled()).WillByDefault([this] { return _shadows; });
      ON_CALL(_render, SetShadowsEnabled(_)).WillByDefault([this](const bool enabled) { _shadows = enabled; return true; });
      ON_CALL(_render, GetShadowMapSize()).WillByDefault([this] { return _shadow_map_size; });
      ON_CALL(_render, SetShadowMapSize(_)).WillByDefault([this](const int size)
      {
        if (!neon::ShadowMapSize::IsSize(size)) { return false; }
        _shadow_map_size = size;
        return true;
      });
      ON_CALL(_render, GetShadowFilter()).WillByDefault([this] { return _shadow_filter; });
      ON_CALL(_render, SetShadowFilter(_)).WillByDefault([this](const ShadowFilter filter) { _shadow_filter = filter; return true; });
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
      return GraphicsMenu(&_ui, &_render, &_window, _presets, player, _logger);
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

    EXPECT_EQ(_values["quality"], "high") << "the defaults are the preset high";
    EXPECT_EQ(_values["window_mode"], "borderless");
    EXPECT_EQ(_values["vsync"], "true");
    EXPECT_EQ(_values["max_fps"], "0");
    EXPECT_EQ(_values["anisotropy"], "8");
    EXPECT_EQ(_values["texture_scale"], "1");
    EXPECT_EQ(_values["target_scale"], "1");
    EXPECT_EQ(_values["target_mipmaps"], "0");
    EXPECT_EQ(_values["shadows"], "true");
    EXPECT_EQ(_values["shadow_map_size"], "2048");
    EXPECT_EQ(_values["shadow_filter"], "pcf");
    EXPECT_EQ(_values["shadow_cascades"], "4");
    EXPECT_EQ(_values["shadow_distance"], "120");
    EXPECT_EQ(_values["tonemapper"], "none");
    EXPECT_EQ(_values["exposure"], "1");
  }

  TEST_F(GraphicsMenuTest, HasAValueForEveryGraphicsSetting)
  {
    EXPECT_EQ(Menu().GetValueNames().size(), 15u);
    EXPECT_EQ(Menu().GetValueNames().front(), "quality") << "the preset first, the rows it sets below it";
  }

  // the preset

  TEST_F(GraphicsMenuTest, OffersThePresetsOfTheTableWhenItIsOpened)
  {
    GraphicsMenu menu = Menu();
    menu.Open();

    EXPECT_THAT(_quality_options, ::testing::ElementsAre("low", "medium", "high", "ultra", "custom"));
    EXPECT_THAT(_logger->Messages(LogLevel::Info), HasSubstr("The menu offers the presets low, medium, high, ultra, custom"));
  }

  TEST_F(GraphicsMenuTest, OffersWhatTheProjectAddedAndNotWhatItDropped)
  {
    EXPECT_TRUE(_presets.Set(GraphicsPreset{.name = "potato", .anisotropy = 1, .shadow_cascades = 1}));
    EXPECT_TRUE(_presets.Remove("ultra"));

    GraphicsMenu menu = Menu();
    menu.Open();

    EXPECT_THAT(_quality_options, ::testing::ElementsAre("low", "medium", "high", "potato", "custom"));
  }

  TEST_F(GraphicsMenuTest, LeavesAMenuWithoutAQualityRowAsItIs)
  {
    ON_CALL(_ui, FindByName(_, _)).WillByDefault([](const std::string &, UiHandle) { return UiHandle{}; });
    EXPECT_CALL(_ui, SetField(_, _, _)).Times(0);

    GraphicsMenu menu = Menu();
    menu.Open();

    EXPECT_TRUE(_quality_options.empty());
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u) << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(GraphicsMenuTest, APresetOfTheProjectIsAppliedAndShownLikeABuiltInOne)
  {
    EXPECT_TRUE(_presets.Set(GraphicsPreset{.name = "potato", .anisotropy = 1, .shadow_map_size = 512, .shadow_cascades = 1}));

    GraphicsMenu menu = Menu(&_player);
    menu.Open();

    _values["quality"] = "potato";
    menu.Update();

    EXPECT_EQ(_anisotropy, 1);
    EXPECT_EQ(_shadow_map_size, 512);
    EXPECT_EQ(_shadow_cascades, 1);
    EXPECT_DOUBLE_EQ(_texture_scale, 1.0) << "what the project left out is the engine's default";
    EXPECT_EQ(_values["quality"], "potato");
    EXPECT_EQ(_values["anisotropy"], "1");

    // the values of the preset, however they came about, show its name
    _values["shadow_cascades"] = "2";
    menu.Update();
    EXPECT_EQ(_values["quality"], "custom");
    _values["shadow_cascades"] = "1";
    menu.Update();
    EXPECT_EQ(_values["quality"], "potato");

    menu.Close(true);
    std::string name;
    ASSERT_NE(Kept("rendering", "quality"), nullptr);
    EXPECT_TRUE(Kept("rendering", "quality")->GetText(name));
    EXPECT_EQ(name, "potato");
  }

  TEST_F(GraphicsMenuTest, APresetTheProjectDroppedIsNoPreset)
  {
    EXPECT_TRUE(_presets.Remove("low"));

    GraphicsMenu menu = Menu();
    menu.Open();
    _values["quality"] = "low";
    menu.Update();

    EXPECT_EQ(_anisotropy, 8);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u) << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(GraphicsMenuTest, APresetSetsEverySettingThatCostsFrameTimeAtOnce)
  {
    GraphicsMenu menu = Menu();
    menu.Open();

    _values["quality"] = "low";
    menu.Update();

    EXPECT_EQ(_anisotropy, 2);
    EXPECT_DOUBLE_EQ(_texture_scale, 0.5);
    EXPECT_DOUBLE_EQ(_target_scale, 0.5);
    EXPECT_EQ(_target_mipmaps, 1);
    EXPECT_EQ(_shadow_map_size, 1024);
    EXPECT_EQ(_shadow_filter, ShadowFilter::None);
    EXPECT_EQ(_shadow_cascades, 1);
    EXPECT_DOUBLE_EQ(_shadow_distance, 40.0);

    EXPECT_TRUE(_vsync) << "what is a matter of taste stays";
    EXPECT_TRUE(_shadows) << "whether shadows are drawn at all is the player's own choice";
    EXPECT_EQ(_tonemapper, Tonemapper::None);
    EXPECT_DOUBLE_EQ(_exposure, 1.0);
    EXPECT_EQ(_mode, WindowMode::Borderless);

    // the rows show what the preset set, and the preset stays what was chosen
    EXPECT_EQ(_values["anisotropy"], "2");
    EXPECT_EQ(_values["texture_scale"], "0.5");
    EXPECT_EQ(_values["shadow_distance"], "40");
    EXPECT_EQ(_values["shadow_map_size"], "1024");
    EXPECT_EQ(_values["shadow_filter"], "none");
    EXPECT_EQ(_values["quality"], "low");
  }

  TEST_F(GraphicsMenuTest, ASettingChangedByHandMakesThePresetCustom)
  {
    GraphicsMenu menu = Menu();
    menu.Open();

    _values["anisotropy"] = "16";
    menu.Update();

    EXPECT_EQ(_anisotropy, 16);
    EXPECT_EQ(_values["quality"], "custom");

    // and the values of a preset, however they came about, show its name
    _values["anisotropy"] = "8";
    menu.Update();
    EXPECT_EQ(_values["quality"], "high");
  }

  TEST_F(GraphicsMenuTest, CustomChangesNothing)
  {
    GraphicsMenu menu = Menu();
    menu.Open();

    _values["quality"] = "custom";
    EXPECT_CALL(_render, SetAnisotropy(_)).Times(0);
    EXPECT_CALL(_render, SetShadows(_, _)).Times(0);
    menu.Update();

    EXPECT_EQ(_values["quality"], "high") << "the values are still those of high, which is what is shown";
  }

  TEST_F(GraphicsMenuTest, KeepsThePresetWithItsValuesWhenTheMenuIsClosedWithApply)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["quality"] = "medium";
    menu.Update();

    menu.Close(true);

    ASSERT_NE(Kept("rendering", "quality"), nullptr);
    std::string name;
    EXPECT_TRUE(Kept("rendering", "quality")->GetText(name));
    EXPECT_EQ(name, "medium");
    EXPECT_EQ(Kept("rendering", "anisotropy"), nullptr) << "the preset names its values, which would be left out next to it";
    EXPECT_EQ(Kept("rendering", "shadow_cascades"), nullptr);
    EXPECT_EQ(Kept("rendering", "texture_scale"), nullptr);
    EXPECT_EQ(Kept("rendering", "exposure"), nullptr);
  }

  TEST_F(GraphicsMenuTest, APresetTakesTheValuesTheFileHeldOutOfIt)
  {
    // what an earlier Apply kept with custom; the format of this test reads
    // no file, so it is set as the menu would have
    _player.Set("rendering", "quality", DataValue::Text("custom"));
    _player.Set("rendering", "anisotropy", DataValue::Number(16));
    _player.Set("rendering", "exposure", DataValue::Number(2));

    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["quality"] = "low";
    menu.Update();
    menu.Close(true);

    std::string name;
    ASSERT_NE(Kept("rendering", "quality"), nullptr);
    EXPECT_TRUE(Kept("rendering", "quality")->GetText(name));
    EXPECT_EQ(name, "low");
    EXPECT_EQ(Kept("rendering", "anisotropy"), nullptr) << "a value the preset decides is gone from the file";
    EXPECT_NE(Kept("rendering", "exposure"), nullptr) << "the rest stays";
  }

  TEST_F(GraphicsMenuTest, ARowChangedByHandIsKeptWithCustom)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["anisotropy"] = "16";
    menu.Update();
    EXPECT_EQ(_values["quality"], "custom");

    menu.Close(true);

    std::string name;
    ASSERT_NE(Kept("rendering", "quality"), nullptr);
    EXPECT_TRUE(Kept("rendering", "quality")->GetText(name));
    EXPECT_EQ(name, "custom") << "so that the value applies over a preset a layer before chose";
    ASSERT_NE(Kept("rendering", "anisotropy"), nullptr);
  }

  TEST_F(GraphicsMenuTest, ARowChangedByHandWhileCustomIsKeptWithCustomToo)
  {
    // the values match no preset when the menu opens, so custom is shown
    // from the start and would not count as a change by itself
    _anisotropy = 16;
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    EXPECT_EQ(_values["quality"], "custom");
    _values["shadow_distance"] = "60";
    menu.Update();

    menu.Close(true);

    std::string name;
    ASSERT_NE(Kept("rendering", "quality"), nullptr);
    EXPECT_TRUE(Kept("rendering", "quality")->GetText(name));
    EXPECT_EQ(name, "custom");
    ASSERT_NE(Kept("rendering", "shadow_distance"), nullptr);
  }

  TEST_F(GraphicsMenuTest, ARowNoPresetDecidesIsKeptWithoutAPreset)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["vsync"] = "false";
    menu.Update();
    menu.Close(true);

    EXPECT_EQ(Kept("rendering", "quality"), nullptr);
    ASSERT_NE(Kept("rendering", "vsync"), nullptr);
  }

  TEST_F(GraphicsMenuTest, PutsBackThePresetThatWasNotKept)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["quality"] = "low";
    menu.Update();

    menu.Close(false);

    EXPECT_EQ(_anisotropy, 8);
    EXPECT_EQ(_shadow_map_size, 2048);
    EXPECT_EQ(_shadow_filter, ShadowFilter::Pcf);
    EXPECT_EQ(_shadow_cascades, 4);
    EXPECT_DOUBLE_EQ(_shadow_distance, 120.0);
    EXPECT_FALSE(_files.Exists("user://settings.yml"));
  }

  TEST_F(GraphicsMenuTest, SaysOnceWhatIsNoPreset)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["quality"] = "best";
    menu.Update();
    menu.Update();

    EXPECT_EQ(_anisotropy, 8);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u) << _logger->Messages(LogLevel::Warn);

    menu.Close(true);
    EXPECT_EQ(Kept("rendering", "quality"), nullptr);
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
    _values["shadows"] = "false";
    _values["shadow_map_size"] = "1024";
    _values["shadow_filter"] = "none";
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
    EXPECT_FALSE(_shadows);
    EXPECT_EQ(_shadow_map_size, 1024);
    EXPECT_EQ(_shadow_filter, ShadowFilter::None);
  }

  TEST_F(GraphicsMenuTest, KeepsTheShadowSettingsAsTheFileWritesThem)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["shadows"] = "false";
    _values["shadow_map_size"] = "512";
    _values["shadow_filter"] = "none";
    menu.Update();

    menu.Close(true);

    bool shadows = true;
    ASSERT_NE(Kept("rendering", "shadows"), nullptr);
    EXPECT_TRUE(Kept("rendering", "shadows")->GetBool(shadows));
    EXPECT_FALSE(shadows) << "a flag, as rendering.vsync is";

    double size = 0.0;
    ASSERT_NE(Kept("rendering", "shadow_map_size"), nullptr);
    EXPECT_TRUE(Kept("rendering", "shadow_map_size")->GetNumber(size));
    EXPECT_DOUBLE_EQ(size, 512.0);

    std::string filter;
    ASSERT_NE(Kept("rendering", "shadow_filter"), nullptr);
    EXPECT_TRUE(Kept("rendering", "shadow_filter")->GetText(filter));
    EXPECT_EQ(filter, "none");
  }

  TEST_F(GraphicsMenuTest, RefusesASizeOfTheShadowMapThatIsNoneAndKeepsTheMap)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["shadow_map_size"] = "3000";
    menu.Update();

    EXPECT_EQ(_shadow_map_size, 2048);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u) << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(GraphicsMenuTest, PutsTheShadowsBackWhenTheyAreNotKept)
  {
    GraphicsMenu menu = Menu(&_player);
    menu.Open();
    _values["shadows"] = "false";
    _values["shadow_map_size"] = "4096";
    menu.Update();
    EXPECT_FALSE(_shadows);

    menu.Close(false);

    EXPECT_TRUE(_shadows);
    EXPECT_EQ(_shadow_map_size, 2048);
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
