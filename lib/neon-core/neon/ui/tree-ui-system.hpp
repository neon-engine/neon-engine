#ifndef TREE_UI_SYSTEM_HPP
#define TREE_UI_SYSTEM_HPP

#include <cstddef>
#include <map>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/input/input-context.hpp>
#include <neon/layout/layout-engine.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-2d-context.hpp>
#include <neon/text/font-rasterizer.hpp>
#include <neon/text/text-shaper.hpp>

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

    /// Where a user interface is laid out and drawn. The window is the
    /// first, and the only one that is no render target.
    struct Surface
    {
      int id = No_Ui_Surface;
      std::string name;

      // in pixels. The window has the size of the frame, which is asked
      // for in every frame
      int width = 0;
      int height = 0;
      float scale = 1.0f;

      int target = No_Render_Target;

      // where the player points, as the game says. The window is told by
      // the input
      bool has_pointer = false;
      float pointer_x = 0.0f;
      float pointer_y = 0.0f;
      bool pointer_is_down = false;

      bool pointer_was_down = false;
      UiElement *pressed = nullptr;
      UiElement *hovered = nullptr;
      bool uses_pointer = false;
    };

    // by what they are known as. A place is empty once its surface is
    // destroyed
    std::vector<std::unique_ptr<Surface>> _surfaces;
    int _input_surface = Ui_Window_Surface;

    // names of surfaces that could not be made, each said once
    std::vector<std::string> _refused_surfaces;

    // the values of one user interface, by the name of the user interface
    std::map<std::string, std::unique_ptr<UiValues>> _values_of;

    // callbacks for an element of one user interface, by both names
    std::map<std::pair<std::string, std::string>, std::function<void()>> _callbacks_in;

    [[nodiscard]] Surface *FindSurface(int surface) const;

    [[nodiscard]] UiValues &ValuesOf(const std::string &interface);

    /// The size of a surface in pixels, which is that of the frame for the
    /// window.
    void SizeOf(const Surface &surface, int &width, int &height) const;

    [[nodiscard]] UiElement *HitTest(int surface, float x, float y) const;

    /// What the pointer of a surface does in this frame.
    void UpdatePointer(Surface &surface, bool has_pointer, float x, float y, bool is_down);

    void Paint(const Surface &surface);
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

    // seconds since the user interface was started
    double _time = 0.0;

    std::size_t _reported_draw_calls = 0;
    std::size_t _quads = 0;

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

    /// What shapes the text from now on: kerning, ligatures, and the
    /// scripts whose letters are joined or run from right to left.
    /// Without one, text is drawn character by character. Call it before
    /// a file is shown.
    void SetTextShaper(TextShaper *shaper);

    /// The fonts and the images, for an application that adds to them.
    [[nodiscard]] UiResources &GetResources();

    /// What reads image files. With one, images have smaller copies and
    /// are smooth when they are drawn smaller than they are. Without one,
    /// the renderer is asked to load them.
    void SetImageDecoder(ImageDecoder *decoder);

    /// What draws images that are made of shapes, such as SVG.
    void SetVectorImageRasterizer(VectorImageRasterizer *rasterizer);

    void AdvanceTime(double seconds) override;

    [[nodiscard]] double GetTime() const override;

    int CreateSurface(const std::string &name, int width, int height, float scale = 1.0f) override;

    void DestroySurface(int surface) override;

    [[nodiscard]] int FindSurface(const std::string &name) const override;

    int LoadOnto(int surface, const std::string &path) override;

    void SetPointer(int surface, float x, float y, bool is_down) override;

    void SetPointerUv(int surface, float u, float v, bool is_down) override;

    void ClearPointer(int surface) override;

    bool SetInputSurface(int surface) override;

    [[nodiscard]] int GetInputSurface() const override;

    void SetNumberOf(const std::string &interface, const std::string &name, double number) override;

    void SetTextOf(const std::string &interface, const std::string &name, const std::string &text) override;

    void SetFlagOf(const std::string &interface, const std::string &name, bool flag) override;

    void OnClickIn(
      const std::string &interface,
      const std::string &element,
      const std::function<void()> &callback) override;

    [[nodiscard]] bool WasClickedIn(const std::string &interface, const std::string &element) const override;

    /// The element of that name in one user interface, or nullptr. For
    /// tools and tests.
    [[nodiscard]] const UiElement *FindIn(const std::string &interface, const std::string &name) const;

    /// The texture a surface is shown with where it is drawn in two
    /// dimensions, or No_Texture. The window has none.
    [[nodiscard]] int GetSurfaceTexture(int surface) const;

    /// What the window is called as a surface.
    static constexpr const char *kWindow_Name = "window";
  };
} // neon

#endif //TREE_UI_SYSTEM_HPP
