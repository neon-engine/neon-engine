#ifndef UI_FIXTURE_HPP
#define UI_FIXTURE_HPP

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/layout/flex-layout-engine.hpp>
#include <neon/testing/fake-font-rasterizer.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-render-2d-context.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/ui/tree-ui-system.hpp>

namespace neon::testing
{
  /// The user interface with everything it needs, and nothing that needs
  /// a machine of a certain kind.
  ///
  /// The font of the tests moves the pen by half its size for every
  /// character, and a line is as high as the size. At the size of 16 a
  /// file starts with, `Start` is 40 wide and 16 high.
  class UiTest : public ::testing::Test
  {
  protected:
    static constexpr const char *kPath = "assets://ui/test.ui.yml";

    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    FakeFontRasterizer _rasterizer;
    Flex_LayoutEngine _layout;
    RecordingRenderer2D _renderer;
    ::testing::NiceMock<FakeInputContext> _input{_logger};
    std::unique_ptr<Tree_UiSystem> _ui;

    void SetUp() override
    {
      _file_system.Initialize();
      _file_system.AddNativeFile("/assets/fonts/regular.ttf", "a font");
      _file_system.AddNativeFile("/assets/fonts/bold.ttf", "a font");

      Create({});
    }

    void TearDown() override
    {
      if (_ui != nullptr) { _ui->CleanUp(); }
    }

    void Create(const std::string &start_path)
    {
      _ui = std::make_unique<Tree_UiSystem>(
        &_renderer,
        &_rasterizer,
        &_layout,
        &_input,
        &_file_system,
        &_yaml,
        UiSettings{
          .fonts = {
            {"sans-serif", 400, "assets://fonts/regular.ttf"},
            {"sans-serif", 700, "assets://fonts/bold.ttf"}
          },
          .start_path = start_path
        },
        _logger);
    }

    void Write(const std::string &path, const std::string &text)
    {
      ASSERT_TRUE(_file_system.WriteText(path, text)) << path;
    }

    /// Writes a file of the scheme assets://, which is read-only to
    /// everything but a test.
    void WriteAsset(const std::string &name, const std::string &text)
    {
      _file_system.AddNativeFile("/assets/" + name, text);
    }

    /// Shows a user interface that is written as YAML.
    int Show(const std::string &yaml, const std::string &name = "ui/test.ui.yml")
    {
      WriteAsset(name, yaml);
      return _ui->Load("assets://" + name);
    }

    /// Shows the elements under a root that fills the frame. Each line of
    /// `children` starts in the first column.
    int ShowUnderRoot(const std::string &children, const std::string &root = "")
    {
      std::string yaml =
        "ui: test\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n";

      yaml += Indented(root, "  ");
      yaml += "  children:\n";
      yaml += Indented(children, "    ");

      return Show(yaml);
    }

    [[nodiscard]] static std::string Indented(const std::string &text, const std::string &by)
    {
      std::string indented;
      std::size_t start = 0;

      while (start < text.size())
      {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) { end = text.size(); }

        const std::string line = text.substr(start, end - start);
        indented += (line.empty() ? "" : by + line) + "\n";
        start = end + 1;
      }

      return indented;
    }

    /// A frame as the runtime runs it. What was drawn in the frame before
    /// is forgotten.
    void Frame()
    {
      _renderer.batches.clear();
      _ui->Update();
      _ui->Draw();
    }

    /// Releases everything, as an input system does where a frame starts.
    void Release()
    {
      _input.state.Reset();
    }

    void PointAt(const double x, const double y)
    {
      _input.state.SetPointer(x, y);
    }

    /// A frame with the button of the pointer held down, and one with it
    /// released.
    void ClickAt(const double x, const double y)
    {
      Release();
      PointAt(x, y);
      _input.state.SetAction(Action::Pointer_Primary);
      Frame();

      Release();
      Frame();
    }

    /// A frame with the action held down, and one with it released.
    void Press(const Action action)
    {
      Release();
      _input.state.SetAction(action);
      Frame();

      Release();
      Frame();
    }

    [[nodiscard]] const UiElement &Element(const std::string &name) const
    {
      const UiElement *element = _ui->Find(name);
      if (element == nullptr) { throw std::runtime_error("There is no element '" + name + "'"); }
      return *element;
    }

    void ExpectBox(
      const std::string &name,
      const float left,
      const float top,
      const float width,
      const float height) const
    {
      const UiRectangle &box = Element(name).GetBox();
      EXPECT_NEAR(box.left, left, 0.01f) << "left of " << name;
      EXPECT_NEAR(box.top, top, 0.01f) << "top of " << name;
      EXPECT_NEAR(box.Width(), width, 0.01f) << "width of " << name;
      EXPECT_NEAR(box.Height(), height, 0.01f) << "height of " << name;
    }

    [[nodiscard]] std::vector<std::string> Errors() const
    {
      std::vector<std::string> errors;
      for (const auto &[level, message] : _logger->Entries())
      {
        if (level == LogLevel::Error) { errors.push_back(message); }
      }
      return errors;
    }

    /// Expects the file to be refused with exactly these problems, in this
    /// order. The line that says how many there are is left out.
    void ExpectProblems(const std::string &yaml, const std::vector<std::string> &problems)
    {
      _logger->Clear();
      EXPECT_EQ(Show(yaml), -1);

      auto errors = Errors();
      ASSERT_FALSE(errors.empty());

      const std::string count = std::to_string(problems.size()) + (problems.size() == 1 ? " problem" : " problems");
      EXPECT_EQ(
        errors.back(),
        std::string("The user interface ") + kPath + " has " + count + " and is not shown");

      errors.pop_back();
      EXPECT_EQ(errors, problems);
    }

    /// The same, for what is wrong with an element under a root.
    void ExpectProblemsUnderRoot(const std::string &children, const std::vector<std::string> &problems)
    {
      ExpectProblems(
        "root:\n"
        "  type: panel\n"
        "  children:\n" + Indented(children, "    "),
        problems);
    }
  };
} // neon::testing

#endif //UI_FIXTURE_HPP
