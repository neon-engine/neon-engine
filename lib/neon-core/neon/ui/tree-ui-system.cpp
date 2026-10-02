#include "tree-ui-system.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>

namespace neon
{
  Tree_UiSystem::Tree_UiSystem(
    Render2DContext *renderer,
    FontRasterizer *rasterizer,
    LayoutEngine *layout,
    InputContext *input,
    FileSystemContext *file_system,
    DocumentFormat *format,
    const UiSettings &settings,
    const std::shared_ptr<Logger> &logger)
    : _draw_cache(renderer),
      _resources(renderer, rasterizer, file_system, logger),
      _file(file_system, format, &_types),
      _painter(&_draw_cache),
      _gate(input, logger)
  {
    _renderer = renderer;
    _layout = layout;
    _input = input;
    _settings = settings;
    _logger = logger;

    _types.AddEngineElements();
    _resources.SetDocumentFormat(format);

    // the window, which is there from the start
    auto window = std::make_unique<Surface>();
    window->id = Ui_Window_Surface;
    window->name = kWindow_Name;
    _surfaces.push_back(std::move(window));

    for (const auto &face : _settings.fonts) { _resources.AddFace(face.family, face.weight, face.path, face.options); }
  }

  UiElementTypes &Tree_UiSystem::GetElementTypes()
  {
    return _types;
  }

  void Tree_UiSystem::SetTextShaper(TextShaper *shaper)
  {
    _resources.SetTextShaper(shaper);
  }

  UiResources &Tree_UiSystem::GetResources()
  {
    return _resources;
  }

  void Tree_UiSystem::SetImageDecoder(ImageDecoder *decoder)
  {
    _resources.SetImageDecoder(decoder);
  }

  void Tree_UiSystem::SetVectorImageRasterizer(VectorImageRasterizer *rasterizer)
  {
    _resources.SetVectorImageRasterizer(rasterizer);
  }

  void Tree_UiSystem::AdvanceTime(const double seconds)
  {
    if (seconds > 0.0) { _time += seconds; }
  }

  double Tree_UiSystem::GetTime() const
  {
    return _time;
  }

  InputContext *Tree_UiSystem::GetGameInput()
  {
    return &_gate;
  }

  void Tree_UiSystem::Initialize()
  {
    _logger->Info("Initializing the user interface");
    _initialized = true;

    if (_settings.start_path.empty()) { return; }

    // the game goes on without it, and the exit code says it was missing
    if (Load(_settings.start_path) < 0)
    {
      _logger->Error("The user interface {} cannot be used, nothing is shown from the start", _settings.start_path);
    }
  }

  void Tree_UiSystem::CleanUp()
  {
    // A file can be shown without Initialize(), so what it brought with it
    // is released either way.
    if (_initialized) { _logger->Info("Cleaning up the user interface"); }
    _initialized = false;

    for (const auto &document : _documents)
    {
      if (document->root != nullptr)
      {
        document->root->DestroyLayout(*_layout);

        // an element must not tell a user interface that is gone
        document->root->SetHost(nullptr);
      }
    }
    _documents.clear();

    // every surface but the window, which stays for what is shown next
    for (std::size_t i = 1; i < _surfaces.size(); i++)
    {
      if (_surfaces[i] != nullptr && _surfaces[i]->target != No_Render_Target)
      {
        _renderer->DestroyRenderTarget(_surfaces[i]->target);
      }
    }
    _surfaces.resize(1);
    _surfaces[0]->pressed = nullptr;
    _surfaces[0]->hovered = nullptr;
    _input_surface = Ui_Window_Surface;
    _refused_surfaces.clear();

    _focused = nullptr;
    _pressed = nullptr;
    _hovered = nullptr;
    _in_state.clear();
    _elements.clear();
    _animator.Clear();
    _noticing.clear();
    _captured = nullptr;
    _scroll_moves.clear();
    _glides.clear();
    _thumb_drag = {};
    _content_drag = {};
    _tooltip = {};
    _told_focus = 0;
    _always_painting = 0;
    _draw_cache.Clear();
    _needs_paint = true;
    _events.clear();
    _resources.CleanUp();
  }

  int Tree_UiSystem::Load(const std::string &path)
  {
    return LoadOnto(Ui_Window_Surface, path);
  }

  int Tree_UiSystem::LoadOnto(const int surface, const std::string &path)
  {
    const Surface *onto = FindSurface(surface);
    if (onto == nullptr)
    {
      _logger->Error("The user interface {} cannot be shown on surface {}, which does not exist", path, surface);
      return -1;
    }

    if (onto->id == Ui_Window_Surface)
    {
      _logger->Info("Loading the user interface from {}", path);
    } else
    {
      _logger->Info("Loading the user interface from {} onto the surface '{}'", path, onto->name);
    }

    std::vector<std::string> errors;
    auto document = _file.Read(path, errors);

    if (document == nullptr || document->root == nullptr)
    {
      for (const auto &error : errors) { _logger->Error("{}", error); }

      const std::size_t count = errors.size();
      const std::string problems = count == 1 ? "problem" : "problems";
      _logger->Error("The user interface {} has {} {} and is not shown", path, count, problems);
      return -1;
    }

    document->id = _next_id++;
    document->surface = surface;
    Adopt(*document);

    UiDocument &loaded = *document;
    _documents.push_back(std::move(document));

    // by now the elements find the document they are in
    Joined(*loaded.root);
    _needs_paint = true;

    FocusAtStart(loaded);
    return loaded.id;
  }

  void Tree_UiSystem::Unload(const int document)
  {
    const auto found = std::ranges::find_if(_documents, [document](const auto &each)
    {
      return each->id == document;
    });

    if (found == _documents.end()) { return; }

    const std::string path = (*found)->path;
    _logger->Info("Unloading the user interface of {}", path);

    // the focus returns to where it was when the file took it
    const bool had_focus = DocumentOf(_focused) == found->get();
    const std::uint64_t remembered = (*found)->remembered_focus;

    if (had_focus) { _focused = nullptr; }
    if (DocumentOf(_pressed) == found->get()) { _pressed = nullptr; }

    for (const auto &surface : _surfaces)
    {
      if (surface == nullptr) { continue; }
      if (DocumentOf(surface->pressed) == found->get()) { surface->pressed = nullptr; }
      if (DocumentOf(surface->hovered) == found->get()) { surface->hovered = nullptr; }
    }

    Leaving(*(*found)->root);

    (*found)->root->DestroyLayout(*_layout);
    (*found)->root->SetHost(nullptr);
    _documents.erase(found);

    if (had_focus)
    {
      if (UiElement *before = ElementOf(UiHandle{remembered});
        before != nullptr && before->IsFocusable() && CanBeUsed(before) && TakesInput(before))
      {
        SetFocus(before, false);
      }
    }

    _needs_paint = true;
  }

  bool Tree_UiSystem::IsShown(const int document) const
  {
    return std::ranges::any_of(_documents, [document](const auto &each) { return each->id == document; });
  }

  void Tree_UiSystem::SetNumber(const std::string &name, const double number)
  {
    _values.Set(name, UiValue::Number(number));
  }

  void Tree_UiSystem::SetText(const std::string &name, const std::string &text)
  {
    _values.Set(name, UiValue::Text(text));
  }

  void Tree_UiSystem::SetFlag(const std::string &name, const bool flag)
  {
    _values.Set(name, UiValue::Flag(flag));
  }

  std::string Tree_UiSystem::GetValue(const std::string &name, bool *is_set) const
  {
    const UiValue *value = _values.Find(name);
    if (is_set != nullptr) { *is_set = value != nullptr; }
    return value != nullptr ? value->AsText() : "";
  }

  bool Tree_UiSystem::GetNumber(const std::string &name, double &number) const
  {
    const UiValue *value = _values.Find(name);
    if (value == nullptr) { return false; }

    // a text that holds no number is told apart by a number no text holds
    const double read = value->AsNumber(std::numeric_limits<double>::quiet_NaN());
    if (std::isnan(read)) { return false; }

    number = read;
    return true;
  }

  void Tree_UiSystem::OnClick(const std::string &element, const std::function<void()> &callback)
  {
    if (callback)
    {
      _callbacks[element] = callback;
    } else
    {
      _callbacks.erase(element);
    }
  }

  const std::vector<UiEvent> &Tree_UiSystem::GetEvents() const
  {
    return _events;
  }

  bool Tree_UiSystem::WasClicked(const std::string &element) const
  {
    return std::ranges::any_of(_events, [&element](const UiEvent &event)
    {
      return event.kind == UiEvent::Kind::Click && event.element == element;
    });
  }

  std::size_t Tree_UiSystem::GetDrawCalls() const
  {
    return _draw_calls;
  }

  std::size_t Tree_UiSystem::FirstActive() const
  {
    // Of what is shown on the window. A surface in the world takes no
    // input away from the game, nor from another surface.
    for (std::size_t i = _documents.size(); i > 0; i--)
    {
      if (_documents[i - 1]->modal && _documents[i - 1]->surface == Ui_Window_Surface) { return i - 1; }
    }
    return 0;
  }

  bool Tree_UiSystem::HasModal() const
  {
    return std::ranges::any_of(_documents, [](const auto &document)
    {
      return document->modal && document->surface == Ui_Window_Surface;
    });
  }

  UiDocument *Tree_UiSystem::DocumentOf(const UiElement *element) const
  {
    if (element == nullptr) { return nullptr; }

    while (element->GetParent() != nullptr) { element = element->GetParent(); }

    for (const auto &document : _documents)
    {
      if (document->root.get() == element) { return document.get(); }
    }
    return nullptr;
  }

  bool Tree_UiSystem::TakesInput(const UiElement *element) const
  {
    const UiDocument *document = DocumentOf(element);
    if (document == nullptr) { return false; }

    // what is shown on a surface in the world is not in the way of a menu
    // on the window, and not held back by one
    if (document->surface != Ui_Window_Surface) { return true; }

    for (std::size_t i = FirstActive(); i < _documents.size(); i++)
    {
      if (_documents[i].get() == document) { return true; }
    }
    return false;
  }

  bool Tree_UiSystem::CanBeUsed(const UiElement *element)
  {
    return element != nullptr && !element->IsHidden() && element->IsEnabled();
  }

  void Tree_UiSystem::Collect(UiElement &element, std::vector<UiElement *> &elements)
  {
    elements.push_back(&element);
    for (const auto &child : element.GetChildren()) { Collect(*child, elements); }
  }

  UiElement *Tree_UiSystem::FindByName(UiElement &element, const std::string &name)
  {
    if (element.GetName() == name) { return &element; }

    for (const auto &child : element.GetChildren())
    {
      if (UiElement *found = FindByName(*child, name); found != nullptr) { return found; }
    }
    return nullptr;
  }

  const UiElement *Tree_UiSystem::Find(const std::string &name) const
  {
    if (name.empty()) { return nullptr; }

    for (std::size_t i = _documents.size(); i > 0; i--)
    {
      if (UiElement *found = FindByName(*_documents[i - 1]->root, name); found != nullptr) { return found; }
    }
    return nullptr;
  }

  bool Tree_UiSystem::Focus(const std::string &element)
  {
    auto *found = const_cast<UiElement *>(Find(element));
    if (found == nullptr || !found->IsFocusable()) { return false; }

    SetFocus(found, true);
    return true;
  }

  std::string Tree_UiSystem::GetFocused() const
  {
    return _focused != nullptr ? _focused->GetName() : "";
  }

  void Tree_UiSystem::FocusAtStart(UiDocument &document)
  {
    std::vector<UiElement *> elements;
    Collect(*document.root, elements);

    // where the focus returns to when the file is gone
    document.remembered_focus = _focused != nullptr ? _focused->GetId() : 0;

    for (UiElement *element : elements)
    {
      if (element->IsFocusable() && element->WantsFocus())
      {
        SetFocus(element, false);
        return;
      }
    }

    // A menu is used with a controller from the moment it is shown, so
    // something in it has the focus. What is shown during play does not
    // take the focus unless it asks for it.
    if (!document.modal) { return; }

    for (UiElement *element : elements)
    {
      if (element->IsFocusable())
      {
        SetFocus(element, false);
        return;
      }
    }
  }

  Tree_UiSystem::Surface *Tree_UiSystem::FindSurface(const int surface) const
  {
    if (surface < 0 || static_cast<std::size_t>(surface) >= _surfaces.size()) { return nullptr; }

    return _surfaces[surface].get();
  }

  void Tree_UiSystem::SizeOf(const Surface &surface, int &width, int &height) const
  {
    if (surface.id == Ui_Window_Surface)
    {
      const auto [frame_width, frame_height] = _renderer->GetRenderResolution();
      width = frame_width;
      height = frame_height;
      return;
    }

    width = surface.width;
    height = surface.height;
  }

  UiValues &Tree_UiSystem::ValuesOf(const std::string &interface)
  {
    auto &values = _values_of[interface];

    if (values == nullptr)
    {
      values = std::make_unique<UiValues>();
      values->SetParent(&_values);
    }

    return *values;
  }

  int Tree_UiSystem::CreateSurface(const std::string &name, const int width, const int height, const float scale)
  {
    // what is wrong is said once for a name, and not in every frame
    const auto refuse = [this, &name](const std::string &reason)
    {
      if (std::ranges::find(_refused_surfaces, name) == _refused_surfaces.end())
      {
        _refused_surfaces.push_back(name);
        _logger->Error("The surface '{}' cannot be created: {}", name, reason);
      }
      return No_Ui_Surface;
    };

    if (name.empty()) { return refuse("it has no name"); }
    if (FindSurface(name) != No_Ui_Surface) { return refuse("there is a surface of that name"); }

    if (width <= 0 || height <= 0)
    {
      return refuse(std::format("its size is {} by {}, where both are above 0", width, height));
    }

    if (scale <= 0.0f) { return refuse(std::format("its scale is {}, where a number above 0 was expected", scale)); }

    const int target = _renderer->CreateRenderTarget(name, width, height);
    if (target == No_Render_Target) { return refuse("the renderer made no render target for it"); }

    auto surface = std::make_unique<Surface>();
    surface->name = name;
    surface->width = width;
    surface->height = height;
    surface->scale = scale;
    surface->target = target;

    // the place of one that was destroyed is taken again
    for (std::size_t i = 1; i < _surfaces.size(); i++)
    {
      if (_surfaces[i] == nullptr)
      {
        surface->id = static_cast<int>(i);
        _surfaces[i] = std::move(surface);
        return static_cast<int>(i);
      }
    }

    surface->id = static_cast<int>(_surfaces.size());
    _surfaces.push_back(std::move(surface));

    std::erase(_refused_surfaces, name);

    // an image of the surface may be waiting for it
    _needs_paint = true;
    return static_cast<int>(_surfaces.size()) - 1;
  }

  void Tree_UiSystem::DestroySurface(const int surface)
  {
    const Surface *found = FindSurface(surface);
    if (found == nullptr || found->id == Ui_Window_Surface) { return; }

    // what is shown on it goes with it
    std::vector<int> shown;
    for (const auto &document : _documents)
    {
      if (document->surface == surface) { shown.push_back(document->id); }
    }
    for (const int document : shown) { Unload(document); }

    _logger->Info("Destroying the surface '{}'", found->name);
    _renderer->DestroyRenderTarget(found->target);

    if (_input_surface == surface)
    {
      _input_surface = Ui_Window_Surface;
      SetFocus(nullptr, false);
    }

    _surfaces[surface].reset();
    _needs_paint = true;
  }

  int Tree_UiSystem::FindSurface(const std::string &name) const
  {
    for (const auto &surface : _surfaces)
    {
      if (surface != nullptr && surface->name == name) { return surface->id; }
    }
    return No_Ui_Surface;
  }

  int Tree_UiSystem::GetSurfaceTexture(const int surface) const
  {
    const Surface *found = FindSurface(surface);
    if (found == nullptr || found->target == No_Render_Target) { return No_Texture; }

    return _renderer->GetRenderTargetTexture(found->target);
  }

  void Tree_UiSystem::SetPointer(const int surface, const float x, const float y, const bool is_down)
  {
    Surface *found = FindSurface(surface);

    // the window is told by the input
    if (found == nullptr || found->id == Ui_Window_Surface) { return; }

    found->has_pointer = true;
    found->pointer_x = x;
    found->pointer_y = y;
    found->pointer_is_down = is_down;
  }

  void Tree_UiSystem::SetPointerUv(const int surface, const float u, const float v, const bool is_down)
  {
    const Surface *found = FindSurface(surface);
    if (found == nullptr) { return; }

    SetPointer(
      surface, u * static_cast<float>(found->width), v * static_cast<float>(found->height), is_down);
  }

  void Tree_UiSystem::ClearPointer(const int surface)
  {
    Surface *found = FindSurface(surface);
    if (found == nullptr || found->id == Ui_Window_Surface) { return; }

    found->has_pointer = false;
    found->pointer_is_down = false;
  }

  bool Tree_UiSystem::SetInputSurface(const int surface)
  {
    if (FindSurface(surface) == nullptr) { return false; }
    if (_input_surface == surface) { return true; }

    _input_surface = surface;

    // what had the focus is on the surface that has lost the input
    SetFocus(nullptr, false);
    _accept_was_down = _input->GetInputState()[Action::Ui_Accept];
    return true;
  }

  int Tree_UiSystem::GetInputSurface() const
  {
    return _input_surface;
  }

  void Tree_UiSystem::SetNumberOf(const std::string &interface, const std::string &name, const double number)
  {
    ValuesOf(interface).Set(name, UiValue::Number(number));
  }

  void Tree_UiSystem::SetTextOf(const std::string &interface, const std::string &name, const std::string &text)
  {
    ValuesOf(interface).Set(name, UiValue::Text(text));
  }

  void Tree_UiSystem::SetFlagOf(const std::string &interface, const std::string &name, const bool flag)
  {
    ValuesOf(interface).Set(name, UiValue::Flag(flag));
  }

  void Tree_UiSystem::OnClickIn(
    const std::string &interface,
    const std::string &element,
    const std::function<void()> &callback)
  {
    if (callback)
    {
      _callbacks_in[{interface, element}] = callback;
    } else
    {
      _callbacks_in.erase({interface, element});
    }
  }

  bool Tree_UiSystem::WasClickedIn(const std::string &interface, const std::string &element) const
  {
    return std::ranges::any_of(_events, [&](const UiEvent &event)
    {
      return event.kind == UiEvent::Kind::Click && event.document == interface && event.element == element;
    });
  }

  const UiElement *Tree_UiSystem::FindIn(const std::string &interface, const std::string &name) const
  {
    if (name.empty()) { return nullptr; }

    for (std::size_t i = _documents.size(); i > 0; i--)
    {
      if (_documents[i - 1]->name != interface) { continue; }

      if (UiElement *found = FindByName(*_documents[i - 1]->root, name); found != nullptr) { return found; }
    }
    return nullptr;
  }

  UiElement *Tree_UiSystem::HitTest(UiElement &element, const float scale, float x, float y)
  {
    if (element.IsHidden() || element.IsClippedAway()) { return nullptr; }

    // The pointer is where the element is drawn. Where that is on the
    // element itself follows from undoing what moved it, which holds for
    // what is inside the element as well.
    if (!element.ToLocal(scale, x, y)) { return nullptr; }

    const UiStyle &style = element.GetStyle();

    // What is cut off cannot be pointed at, and neither can what is
    // hidden. Contains() follows the round corners.
    const bool inside = element.Contains(scale, x, y) &&
                        ToPixels(element.GetVisibleBox(), scale).Contains(x, y) &&
                        style.visibility == UiVisibility::Visible;

    const UiRectangle padding_box = ToPixels(element.GetPaddingBox(), scale);
    const bool children_can_be_hit =
      (!style.ClipsX() || (x >= padding_box.left && x < padding_box.right)) &&
      (!style.ClipsY() || (y >= padding_box.top && y < padding_box.bottom)) &&
      (!(style.ClipsX() && style.ClipsY()) || element.ContainsInPadding(scale, x, y));

    if (children_can_be_hit)
    {
      // what is drawn last is on top, and is asked first
      const auto children = element.GetChildrenInPaintOrder();
      for (std::size_t i = children.size(); i > 0; i--)
      {
        if (UiElement *hit = HitTest(*children[i - 1], scale, x, y); hit != nullptr) { return hit; }
      }
    }

    return inside && style.pointer_events == UiPointerEvents::Auto ? &element : nullptr;
  }

  UiElement *Tree_UiSystem::HitTest(const float x, const float y) const
  {
    return HitTest(Ui_Window_Surface, x, y);
  }

  UiElement *Tree_UiSystem::HitTest(const int surface, const float x, const float y) const
  {
    if (_documents.empty()) { return nullptr; }

    // what is drawn on top of everything, such as the list of a select
    if (_focused != nullptr && _focused->HasTopLayer())
    {
      if (const UiDocument *document = DocumentOf(_focused);
        document != nullptr && document->surface == surface && _focused->TopLayerContains(document->frame, x, y))
      {
        return _focused;
      }
    }

    // on the window, what lies below a menu cannot be pointed at
    const std::size_t first = surface == Ui_Window_Surface ? FirstActive() : 0;

    for (std::size_t i = _documents.size(); i > first; i--)
    {
      const UiDocument &document = *_documents[i - 1];
      if (document.surface != surface) { continue; }

      if (UiElement *hit = HitTest(*document.root, document.frame.scale, x, y); hit != nullptr) { return hit; }
    }
    return nullptr;
  }

  void Tree_UiSystem::UpdatePointer(
    Surface &surface,
    const bool has_pointer,
    const float x,
    const float y,
    const bool is_down)
  {
    if (surface.pressed != nullptr && (!CanBeUsed(surface.pressed) || !TakesInput(surface.pressed)))
    {
      surface.pressed = nullptr;
    }

    surface.hovered = has_pointer ? HitTest(surface.id, x, y) : nullptr;

    const bool pointer_is_down = has_pointer && is_down;

    if (pointer_is_down && !surface.pointer_was_down)
    {
      UiElement *hovered = surface.hovered;
      surface.pressed = hovered != nullptr && hovered->IsClickable() && hovered->IsEnabled() ? hovered : nullptr;

      // what is pressed takes the focus where the keys are
      if (surface.pressed != nullptr && surface.pressed->IsFocusable() && surface.id == _input_surface)
      {
        SetFocus(surface.pressed, false);
      }
    }

    // what it is over, and what it holds down, which includes the frame
    // it lets go in
    surface.uses_pointer = surface.hovered != nullptr || surface.pressed != nullptr;

    if (!pointer_is_down && surface.pointer_was_down)
    {
      // a click is a press and a release on the same element
      if (surface.pressed != nullptr && surface.hovered == surface.pressed) { Click(*surface.pressed, true); }
      surface.pressed = nullptr;
    }

    surface.pointer_was_down = pointer_is_down;
  }

  void Tree_UiSystem::Click(UiElement &element, const bool by_pointer)
  {
    // what is chosen does what it does when it is chosen: a checkbox is
    // ticked
    UiInteraction interaction;
    interaction.kind = UiInteraction::Kind::Accept;
    interaction.by_pointer = by_pointer;
    interaction.x = _pointer_x / ScaleOf(&element);
    interaction.y = _pointer_y / ScaleOf(&element);
    Interact(element, interaction);

    UiElementEvent event;
    event.x = interaction.x;
    event.y = interaction.y;
    event.clicks = _press_count;
    Emit(element, "click", event);

    // a button that closes its file does so once the click is told, when
    // what cancel closes is closed
    const UiDocument *document = DocumentOf(&element);
    if (element.ClosesItsFile() && document != nullptr) { _documents_to_close.push_back(document->id); }

    if (element.GetName().empty()) { return; }
    const Surface *surface = document != nullptr ? FindSurface(document->surface) : nullptr;

    UiEvent clicked;
    clicked.kind = UiEvent::Kind::Click;
    clicked.scene = element.AsksForScene();
    clicked.element = element.GetName();
    clicked.document = document != nullptr ? document->name : "";
    clicked.surface = surface != nullptr ? surface->name : "";
    _events.push_back(clicked);
  }

  bool Tree_UiSystem::MoveFocus(const Direction direction)
  {
    // of the surface that has the keys and the controller
    std::vector<UiElement *> candidates;
    const std::size_t first = _input_surface == Ui_Window_Surface ? FirstActive() : 0;

    for (std::size_t i = first; i < _documents.size(); i++)
    {
      if (_documents[i]->surface != _input_surface) { continue; }

      std::vector<UiElement *> elements;
      Collect(*_documents[i]->root, elements);

      for (UiElement *element : elements)
      {
        if (element->IsFocusable() && CanBeUsed(element)) { candidates.push_back(element); }
      }
    }

    if (candidates.empty()) { return false; }

    if (_focused == nullptr)
    {
      SetFocus(candidates.front(), true);
      return true;
    }

    // in pixels, since the files that are shown may differ in scale
    const auto center_of = [this](const UiElement *element)
    {
      const UiDocument *document = DocumentOf(element);
      const UiRectangle box = ToPixels(element->GetBox(), document != nullptr ? document->frame.scale : 1.0f);
      return std::pair{(box.left + box.right) / 2.0f, (box.top + box.bottom) / 2.0f};
    };

    const auto [from_x, from_y] = center_of(_focused);

    UiElement *nearest = nullptr;
    float least = std::numeric_limits<float>::max();

    for (UiElement *candidate : candidates)
    {
      if (candidate == _focused) { continue; }

      const auto [x, y] = center_of(candidate);
      const float right = x - from_x;
      const float down = y - from_y;

      float along = 0.0f;
      float across = 0.0f;

      switch (direction)
      {
        case Direction::Up:
          along = -down;
          across = right;
          break;
        case Direction::Right:
          along = right;
          across = down;
          break;
        case Direction::Down:
          along = down;
          across = right;
          break;
        case Direction::Left:
          along = -right;
          across = down;
          break;
      }

      if (along <= 0.5f) { continue; }

      // what lies straight ahead is preferred to what is nearer and off
      // to the side
      const float distance = along + 2.0f * std::abs(across);
      if (distance < least)
      {
        least = distance;
        nearest = candidate;
      }
    }

    // nothing in that direction leaves the focus where it is
    if (nearest == nullptr) { return false; }

    SetFocus(nearest, true);
    return true;
  }

  void Tree_UiSystem::Paint(const Surface &surface)
  {
    int width = 0;
    int height = 0;
    SizeOf(surface, width, height);

    _painter.Begin(width, height);
    _painter.SetTime(static_cast<float>(_time));

    for (const auto &document : _documents)
    {
      if (document->surface != surface.id) { continue; }

      document->root->Paint(_painter, document->frame, 1.0f);
      document->needs_paint = false;
    }

    // what is drawn on top of everything, such as the list of a select
    if (_focused != nullptr && _focused->HasTopLayer())
    {
      if (const UiDocument *document = DocumentOf(_focused);
        document != nullptr && document->surface == surface.id)
      {
        _focused->PaintTopLayer(_painter, document->frame);
      }
    }

    // next to the pointer, which is on the window
    if (surface.id == Ui_Window_Surface) { PaintTooltip(); }

    _painter.End();
    _draw_calls += _painter.GetDrawCalls();
    _quads += _painter.GetQuads();

    // what a shader draws changes with time and with the values of the
    // game, so it is drawn again in the next frame
    if (_painter.UsedMaterials()) { _needs_paint = true; }
  }

  void Tree_UiSystem::Draw()
  {
    if (_documents.empty()) { return; }

    // once more, since a state may have changed the size of an element
    Settle();

    const auto [width, height] = _renderer->GetRenderResolution();
    if (width != _painted_width || height != _painted_height) { _needs_paint = true; }

    // A surface in the world is drawn into its image in every frame. What
    // was drawn the frame before is kept for the window alone.
    const bool has_surfaces = std::ranges::any_of(_documents, [](const auto &document)
    {
      return document->surface != Ui_Window_Surface;
    });

    // What was drawn the frame before is what is drawn now, when nothing
    // changed. The renderer is handed it again, and nothing is built.
    if (!_needs_paint && _always_painting == 0 && !has_surfaces)
    {
      _statistics.replays++;
      _draw_calls = _draw_cache.Replay();
      return;
    }

    _painted_width = width;
    _painted_height = height;
    _needs_paint = false;
    _statistics.paints++;

    _draw_calls = 0;
    _quads = 0;
    _draw_cache.Begin();

    // The surfaces in the world first, each into its image, so that they
    // are finished when the frame is, which shows them.
    for (std::size_t i = 1; i < _surfaces.size(); i++)
    {
      if (_surfaces[i] == nullptr) { continue; }

      const Surface &surface = *_surfaces[i];

      const bool is_shown = std::ranges::any_of(_documents, [&surface](const auto &document)
      {
        return document->surface == surface.id;
      });
      if (!is_shown) { continue; }

      // see-through where nothing is drawn, so that what shows the
      // surface decides what is behind it
      if (!_renderer->BeginRenderTarget(surface.target, {0.0f, 0.0f, 0.0f, 0.0f})) { continue; }

      Paint(surface);
      _renderer->EndRenderTarget();
    }

    Paint(*_surfaces[Ui_Window_Surface]);
    _draw_cache.End();

    // an image of shapes that is asked for at a new size is drawn once it
    // has been asked for in a few frames in a row, which takes frames
    if (_resources.IsSettling()) { _needs_paint = true; }

    // said when it changes, and not in every frame
    if (_draw_calls != _reported_draw_calls)
    {
      _reported_draw_calls = _draw_calls;
      _logger->Debug("The user interface is drawn with {} draw calls and {} rectangles", _draw_calls, _quads);
    }
  }
} // neon
