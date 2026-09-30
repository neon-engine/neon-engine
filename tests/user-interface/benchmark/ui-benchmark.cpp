// How long a frame of the user interface takes, with everything the engine
// needs but a graphics card. It is not a test: it prints numbers, and is
// built and run by hand:
//
//   cmake --build --preset macos-arm64-debug --target user-interface.benchmark
//   ./build/macos-arm64-debug/tests/user-interface.benchmark/user-interface.benchmark
//
// Two files are measured: the HUD of the demo, and a list of 2000
// elements. For each, a frame where nothing changed, one where one value
// of the game changed, and one where everything is laid out and drawn
// again, which is what every frame did before layout was done only when
// something was dirty.

#include <chrono>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>

#include <neon/data/ryml-document-format.hpp>
#include <neon/layout/flex-layout-engine.hpp>
#include <neon/testing/fake-font-rasterizer.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-render-2d-context.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/ui/tree-ui-system.hpp>

namespace
{
  using namespace neon;
  using namespace neon::testing;

  struct Bench
  {
    std::shared_ptr<RecordingLogger> logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem file_system{SettingsConfig{}, logger};
    RYML_DocumentFormat yaml;
    FakeFontRasterizer rasterizer;
    Flex_LayoutEngine layout;
    RecordingRenderer2D renderer;
    ::testing::NiceMock<FakeInputContext> input{logger};
    std::unique_ptr<Tree_UiSystem> ui;

    Bench()
    {
      file_system.Initialize();
      file_system.AddNativeFile("/assets/fonts/regular.ttf", "a font");
      file_system.AddNativeFile("/assets/fonts/bold.ttf", "a font");
      renderer.SetResolution(1920, 1080);

      ui = std::make_unique<Tree_UiSystem>(
        &renderer, &rasterizer, &layout, &input, &file_system, &yaml,
        UiSettings{
          .fonts = {
            {"sans-serif", 400, "assets://fonts/regular.ttf"},
            {"sans-serif", 700, "assets://fonts/bold.ttf"}
          }
        },
        logger);
    }

    ~Bench()
    {
      ui->CleanUp();
    }

    void Show(const std::string &name, const std::string &text)
    {
      file_system.AddNativeFile("/assets/ui/" + name, text);
      if (ui->Load("assets://ui/" + name) < 0)
      {
        std::printf("%s\n", logger->Messages(LogLevel::Error).c_str());
        std::exit(1);
      }
    }

    void Frame()
    {
      renderer.batches.clear();
      ui->Update();
      ui->Draw();
    }

    /// Microseconds a frame takes on average, after a warm-up.
    double Measure(const std::function<void()> &before_each, const int frames = 200)
    {
      for (int i = 0; i < 10; i++)
      {
        before_each();
        Frame();
      }

      const auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < frames; i++)
      {
        before_each();
        Frame();
      }
      const auto end = std::chrono::steady_clock::now();

      return std::chrono::duration<double, std::micro>(end - start).count() / frames;
    }

    /// Changes the root in a way that lays out everything under it, as
    /// every frame did before layout was done only where something is
    /// dirty.
    void ChangeTheRoot(const int frame)
    {
      for (const auto &document : ui->GetDocuments())
      {
        ui->Set(UiHandle{document->root->GetId()}, "padding-left", frame % 2 == 0 ? "0px" : "1px");
      }
    }

    std::size_t CountElements()
    {
      std::size_t count = 0;
      for (const auto &document : ui->GetDocuments()) { count += ui->Query("*", UiHandle{document->root->GetId()}).size() + 1; }
      return count;
    }
  };

  std::string Hud()
  {
    return
      "ui: hud\n"
      "reference_size: [1920, 1080]\n"
      "values:\n"
      "  health: 75\n"
      "  player: Ada\n"
      "root:\n"
      "  type: panel\n"
      "  width: 100%\n"
      "  height: 100%\n"
      "  children:\n"
      "    - type: panel\n"
      "      position: absolute\n"
      "      top: 24\n"
      "      left: 24\n"
      "      gap: 8\n"
      "      flex_direction: column\n"
      "      children:\n"
      "        - type: panel\n"
      "          gap: 12\n"
      "          align_items: center\n"
      "          children:\n"
      "            - {type: panel, width: 32, height: 32, background_color: \"#e5484d\"}\n"
      "            - {type: label, name: health, text: \"Health: {health}\", font_size: 28, font_weight: bold}\n"
      "        - {type: bar, value: \"{health}\", max: 100, width: 240, height: 12, background_color: \"#00000080\"}\n"
      "        - {type: label, text: \"{player}\", font_size: 16}\n"
      "    - type: panel\n"
      "      position: absolute\n"
      "      bottom: 32\n"
      "      right: 32\n"
      "      padding: 24\n"
      "      gap: 12\n"
      "      flex_direction: column\n"
      "      background_color: \"#2e3440\"\n"
      "      children:\n"
      "        - {type: label, text: Paused, font_size: 20, text_align: center}\n"
      "        - {type: button, name: resume, text: Resume, autofocus: true, min_width: 160}\n"
      "        - {type: button, name: quit, text: Quit}\n";
  }

  std::string Large(const int rows)
  {
    std::string yaml =
      "ui: large\n"
      "reference_size: [1920, 1080]\n"
      "styles: [large.css]\n"
      "values:\n"
      "  score: 0\n"
      "root:\n"
      "  type: panel\n"
      "  class: root\n"
      "  children:\n"
      "    - type: panel\n"
      "      class: list\n"
      "      name: list\n"
      "      children:\n";

    // each row is four elements: the row, a label, a bar, and a button
    for (int i = 0; i < rows; i++)
    {
      const std::string n = std::to_string(i);
      yaml +=
        "        - type: panel\n"
        "          class: row\n"
        "          children:\n"
        "            - {type: label, class: name, text: \"Item " + n + "\"}\n"
        "            - {type: bar, class: bar, value: " + std::to_string(i % 100) + ", max: 100}\n"
        "            - {type: button, class: use, name: use-" + n + ", text: Use}\n";
    }

    yaml +=
      "    - {type: label, name: score, class: score, text: \"Score: {score}\"}\n";
    return yaml;
  }

  const char *large_css =
    ".root { width: 100%; height: 100%; flex-direction: column; }\n"
    ".list { flex-direction: column; flex-grow: 1; overflow-y: auto; gap: 2px; }\n"
    ".row { align-items: center; gap: 8px; padding: 2px 8px; background-color: #3b4252; }\n"
    ".row:hover { background-color: #4c566a; }\n"
    ".name { width: 200px; }\n"
    ".bar { width: 200px; height: 8px; }\n"
    ".use { min-width: 80px; padding: 2px 8px; }\n"
    ".score { font-size: 24px; }\n";

  void Report(const char *what, Bench &bench)
  {
    const std::size_t elements = bench.CountElements();

    const double untouched = bench.Measure([] {});
    const double one_value = bench.Measure([&bench, i = 0]() mutable { bench.ui->SetNumber("score", i++); });
    const double one_value_health = bench.Measure([&bench, i = 0]() mutable { bench.ui->SetNumber("health", i++ % 100); });
    const double hover = bench.Measure([&bench, i = 0]() mutable
    {
      // the pointer moves between two rows, which changes their state
      bench.input.state.SetPointer(100.0, i++ % 2 == 0 ? 30.0 : 60.0);
    });
    const double everything = bench.Measure([&bench, i = 0]() mutable { bench.ChangeTheRoot(i++); });

    const UiStatistics &statistics = bench.ui->GetStatistics();

    std::printf("%s: %zu elements\n", what, elements);
    std::printf("  nothing changed:               %8.1f us a frame (drawn again from the cache)\n", untouched);
    std::printf("  one value of the game changed: %8.1f us a frame (score)\n", one_value);
    std::printf("                                 %8.1f us a frame (health)\n", one_value_health);
    std::printf("  the pointer moved to another row: %5.1f us a frame (hover)\n", hover);
    std::printf("  everything laid out and drawn: %8.1f us a frame (as every frame was before)\n", everything);
    std::printf(
      "  in all: %zu layouts, of which %zu of everything and %zu spared, %zu elements placed, %zu paints, "
      "%zu replays\n\n",
      statistics.layouts, statistics.full_layouts, statistics.layouts_spared, statistics.laid_out_elements,
      statistics.paints, statistics.replays);
  }
}

int main()
{
  {
    Bench bench;
    bench.Show("hud.ui.yml", Hud());
    bench.Frame();
    Report("HUD of the demo", bench);
  }

  {
    Bench bench;
    bench.file_system.AddNativeFile("/assets/ui/large.css", large_css);
    bench.Show("large.ui.yml", Large(500));
    bench.Frame();
    Report("A list of 500 rows", bench);
  }

  return 0;
}
