#ifndef TREE_UI_SYSTEM_HPP
#define TREE_UI_SYSTEM_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/input/input-context.hpp>
#include <neon/layout/layout-engine.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/text/font-rasterizer.hpp>

#include "ui-document.hpp"
#include "ui-element-types.hpp"
#include "ui-file.hpp"
#include "ui-input-gate.hpp"
#include "ui-painter.hpp"
#include "ui-resources.hpp"
#include "ui-system.hpp"
#include "ui-values.hpp"

namespace neon
{
  /// What an application decides about its user interface.
  struct UiSettings
  {
    /// The fonts every file can ask for without naming them under `fonts`.
    /// The family `sans-serif` is what text is drawn with when a file says
    /// nothing about its font.
    std::vector<UiFontFace> fonts;

    /// Virtual path of a file that is shown from the start. Empty shows
    /// none.
    std::string start_path;
  };

  /// The user interface as a tree of elements for every file that is
  /// shown.
  ///
  /// The elements are objects of their own and not entities of the world.
  /// The order of elements decides what is drawn on top and where the
  /// focus goes, their sizes follow from each other up and down the tree,
  /// and each kind brings behaviour of its own. An entity store keeps
  /// neither an order nor behaviour. A menu also outlives the scene it is
  /// shown over, and is not part of what a scene saves.
  ///
  /// Files that are shown lie on top of each other, the one loaded last on
  /// top. One that is `modal` takes the input away from the game and from
  /// the files below it.
  // ReSharper disable once CppInconsistentNaming
  class Tree_UiSystem final : public UiSystem
  {
    enum class Direction
    {
      Up = 0,
      Right,
      Down,
      Left
    };

    Render2DContext *_renderer;
    LayoutEngine *_layout;
    InputContext *_input;
    UiSettings _settings;
    std::shared_ptr<Logger> _logger;

    UiElementTypes _types;
    UiValues _values;
    UiResources _resources;
    UiFile _file;
    UiPainter _painter;
    UiInputGate _gate;

    // from the bottom to the top
    std::vector<std::unique_ptr<UiDocument>> _documents;
    int _next_id = 0;

    std::vector<UiEvent> _events;
    std::map<std::string, std::function<void()>> _callbacks;

    UiElement *_focused = nullptr;
    UiElement *_pressed = nullptr;

    // what was held down in the frame before, to tell when it is pressed
    bool _pointer_was_down = false;
    bool _accept_was_down = false;
    bool _direction_was_down[4] = {false, false, false, false};

    bool _initialized = false;
    std::size_t _draw_calls = 0;

    /// The first of the documents that take input: the topmost that is
    /// modal, or the one at the bottom.
    [[nodiscard]] std::size_t FirstActive() const;

    [[nodiscard]] bool HasModal() const;

    [[nodiscard]] UiDocument *DocumentOf(const UiElement *element) const;

    [[nodiscard]] bool TakesInput(const UiElement *element) const;

    [[nodiscard]] static bool CanBeUsed(const UiElement *element);

    void Arrange();

    [[nodiscard]] UiElement *HitTest(float x, float y) const;

    [[nodiscard]] static UiElement *HitTest(UiElement &element, float scale, float x, float y);

    void MoveFocus(Direction direction);

    void Click(const UiElement &element);

    void ApplyStates(UiElement &element, const UiElement *hovered, bool takes_input, bool accept_is_down) const;

    static void Collect(UiElement &element, std::vector<UiElement *> &elements);

    [[nodiscard]] static UiElement *FindByName(UiElement &element, const std::string &name);

    void FocusAtStart(UiDocument &document);

  public:
    /// `input` is the input as it comes from the devices. `format` says
    /// which format the files have.
    Tree_UiSystem(
      Render2DContext *renderer,
      FontRasterizer *rasterizer,
      LayoutEngine *layout,
      InputContext *input,
      FileSystemContext *file_system,
      DocumentFormat *format,
      const UiSettings &settings,
      const std::shared_ptr<Logger> &logger);

    /// The kinds of elements a file can hold. Those of the engine are
    /// known. A game adds its own before it loads a file that uses them.
    [[nodiscard]] UiElementTypes &GetElementTypes();

    /// Throws when the file that is shown from the start cannot be used.
    void Initialize() override;

    void Update() override;

    void Draw() override;

    void CleanUp() override;

    [[nodiscard]] InputContext *GetGameInput() override;

    int Load(const std::string &path) override;

    void Unload(int document) override;

    void SetNumber(const std::string &name, double number) override;

    void SetText(const std::string &name, const std::string &text) override;

    void SetFlag(const std::string &name, bool flag) override;

    void OnClick(const std::string &element, const std::function<void()> &callback) override;

    [[nodiscard]] const std::vector<UiEvent> &GetEvents() const override;

    [[nodiscard]] bool WasClicked(const std::string &element) const override;

    bool Focus(const std::string &element) override;

    [[nodiscard]] std::string GetFocused() const override;

    /// The element of that name in the topmost file that has one, or
    /// nullptr. For tools and tests. A game hands over values and does not
    /// reach for elements.
    [[nodiscard]] const UiElement *Find(const std::string &name) const;

    /// Draw calls of the last frame.
    [[nodiscard]] std::size_t GetDrawCalls() const;
  };
} // neon

#endif //TREE_UI_SYSTEM_HPP
