#include "tree-ui-system.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <stdexcept>

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
    : _resources(renderer, rasterizer, file_system, logger),
      _file(file_system, format, &_types),
      _painter(renderer),
      _gate(input, logger)
  {
    _renderer = renderer;
    _layout = layout;
    _input = input;
    _settings = settings;
    _logger = logger;

    _types.AddEngineElements();

    for (const auto &[family, weight, path] : _settings.fonts) { _resources.AddFace(family, weight, path); }
  }

  UiElementTypes &Tree_UiSystem::GetElementTypes()
  {
    return _types;
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

    if (Load(_settings.start_path) < 0)
    {
      throw std::runtime_error("The user interface " + _settings.start_path + " cannot be used");
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
      if (document->root != nullptr) { document->root->DestroyLayout(*_layout); }
    }
    _documents.clear();

    _focused = nullptr;
    _pressed = nullptr;
    _events.clear();
    _resources.CleanUp();
  }

  int Tree_UiSystem::Load(const std::string &path)
  {
    _logger->Info("Loading the user interface from {}", path);

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
    document->frame.resources = &_resources;
    document->frame.values = &_values;

    for (const auto &[family, weight, font_path] : document->fonts) { _resources.AddFace(family, weight, font_path); }
    for (const auto &[name, value] : document->values) { _values.SetDefault(name, value); }

    document->root->CreateLayout(*_layout, &document->frame);

    UiDocument &loaded = *document;
    _documents.push_back(std::move(document));

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

    if (DocumentOf(_focused) == found->get()) { _focused = nullptr; }
    if (DocumentOf(_pressed) == found->get()) { _pressed = nullptr; }

    (*found)->root->DestroyLayout(*_layout);
    _documents.erase(found);
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
    for (std::size_t i = _documents.size(); i > 0; i--)
    {
      if (_documents[i - 1]->modal) { return i - 1; }
    }
    return 0;
  }

  bool Tree_UiSystem::HasModal() const
  {
    return std::ranges::any_of(_documents, [](const auto &document) { return document->modal; });
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

    _focused = found;
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

    for (UiElement *element : elements)
    {
      if (element->IsFocusable() && element->WantsFocus())
      {
        _focused = element;
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
        _focused = element;
        return;
      }
    }
  }

  void Tree_UiSystem::Arrange()
  {
    const auto [width, height] = _renderer->GetRenderResolution();

    for (const auto &document : _documents)
    {
      const float scale = document->ScaleFor(width, height);
      document->frame.scale = scale;

      document->root->Prepare(*_layout, document->frame, false);
      _layout->Calculate(
        document->root->GetLayoutNode(),
        static_cast<float>(width) / scale,
        static_cast<float>(height) / scale);
      document->root->Arrange(*_layout, 0.0f, 0.0f);
    }

    for (const auto &name : _values.TakeMissed())
    {
      _logger->Warn("The value '{}' is not set. What refers to it shows its name, or its default", name);
    }
  }

  UiElement *Tree_UiSystem::HitTest(UiElement &element, const float scale, const float x, const float y)
  {
    if (element.IsHidden()) { return nullptr; }

    const UiStyle &style = element.GetStyle();
    const bool inside = ToPixels(element.GetBox(), scale).Contains(x, y);

    // what is cut off cannot be pointed at
    const bool children_can_be_hit =
      style.overflow != UiOverflow::Hidden || ToPixels(element.GetPaddingBox(), scale).Contains(x, y);

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
    if (_documents.empty()) { return nullptr; }

    const std::size_t first = FirstActive();
    for (std::size_t i = _documents.size(); i > first; i--)
    {
      const UiDocument &document = *_documents[i - 1];
      if (UiElement *hit = HitTest(*document.root, document.frame.scale, x, y); hit != nullptr) { return hit; }
    }
    return nullptr;
  }

  void Tree_UiSystem::Click(const UiElement &element)
  {
    if (element.GetName().empty()) { return; }

    const UiDocument *document = DocumentOf(&element);
    _events.push_back({UiEvent::Kind::Click, element.GetName(), document != nullptr ? document->name : ""});
  }

  void Tree_UiSystem::MoveFocus(const Direction direction)
  {
    std::vector<UiElement *> candidates;
    for (std::size_t i = FirstActive(); i < _documents.size(); i++)
    {
      std::vector<UiElement *> elements;
      Collect(*_documents[i]->root, elements);

      for (UiElement *element : elements)
      {
        if (element->IsFocusable() && CanBeUsed(element)) { candidates.push_back(element); }
      }
    }

    if (candidates.empty()) { return; }

    if (_focused == nullptr)
    {
      _focused = candidates.front();
      return;
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
    if (nearest != nullptr) { _focused = nearest; }
  }

  void Tree_UiSystem::ApplyStates(
    UiElement &element,
    const UiElement *hovered,
    const bool takes_input,
    const bool accept_is_down) const
  {
    UiStates states = element.GetStates();

    bool is_hovered = false;
    for (const UiElement *each = hovered; each != nullptr; each = each->GetParent())
    {
      if (each == &element) { is_hovered = true; }
    }

    states.hover = takes_input && is_hovered;
    states.focus = takes_input && &element == _focused;
    states.active = takes_input &&
                    ((&element == _pressed && hovered == _pressed) ||
                     (&element == _focused && accept_is_down));

    element.SetStates(states);

    for (const auto &child : element.GetChildren())
    {
      ApplyStates(*child, hovered, takes_input, accept_is_down);
    }
  }

  void Tree_UiSystem::Update()
  {
    _events.clear();

    const InputState &input = _input->GetInputState();
    const bool modal = HasModal();

    if (_documents.empty())
    {
      _gate.SetNeedsPointer(false);
      _gate.Refresh({});
      _pointer_was_down = input[Action::Pointer_Primary];
      _accept_was_down = input[Action::Ui_Accept];
      return;
    }

    // the cursor is shown before the pointer is asked for
    _gate.SetNeedsPointer(modal);

    Arrange();

    // what can no longer be used loses the focus
    if (_focused != nullptr && (!CanBeUsed(_focused) || !TakesInput(_focused))) { _focused = nullptr; }
    if (_pressed != nullptr && (!CanBeUsed(_pressed) || !TakesInput(_pressed))) { _pressed = nullptr; }

    UiConsumed consumed;
    consumed.everything = modal;

    // the pointer
    UiElement *hovered = nullptr;
    if (input.HasPointer())
    {
      hovered = HitTest(static_cast<float>(input.GetPointer().x), static_cast<float>(input.GetPointer().y));
    }

    const bool pointer_is_down = input.HasPointer() && input[Action::Pointer_Primary];

    if (pointer_is_down && !_pointer_was_down)
    {
      _pressed = hovered != nullptr && hovered->IsClickable() && hovered->IsEnabled() ? hovered : nullptr;
      if (_pressed != nullptr && _pressed->IsFocusable()) { _focused = _pressed; }
    }

    consumed.pointer = hovered != nullptr || _pressed != nullptr;

    if (!pointer_is_down && _pointer_was_down)
    {
      // a click is a press and a release on the same element
      if (_pressed != nullptr && hovered == _pressed) { Click(*_pressed); }
      _pressed = nullptr;
    }

    _pointer_was_down = pointer_is_down;

    // the keys and the controller
    constexpr Action directions[4] = {Action::Ui_Up, Action::Ui_Right, Action::Ui_Down, Action::Ui_Left};
    for (std::size_t i = 0; i < 4; i++)
    {
      const bool is_down = input[directions[i]];
      if (is_down && !_direction_was_down[i]) { MoveFocus(static_cast<Direction>(i)); }
      _direction_was_down[i] = is_down;
    }

    const bool accept_is_down = input[Action::Ui_Accept];
    if (accept_is_down && !_accept_was_down && CanBeUsed(_focused) && _focused->IsClickable())
    {
      Click(*_focused);
    }
    _accept_was_down = accept_is_down;

    consumed.navigation = _focused != nullptr;

    const std::size_t first = FirstActive();
    for (std::size_t i = 0; i < _documents.size(); i++)
    {
      ApplyStates(*_documents[i]->root, hovered, i >= first, accept_is_down);
    }

    _gate.Refresh(consumed);

    // Called last, and from a copy. A callback is free to load and unload
    // files, and to cause events of its own.
    const std::vector<UiEvent> events = _events;
    for (const auto &event : events)
    {
      if (const auto found = _callbacks.find(event.element); found != _callbacks.end())
      {
        const std::function<void()> callback = found->second;
        callback();
      }
    }
  }

  void Tree_UiSystem::Draw()
  {
    _draw_calls = 0;
    if (_documents.empty()) { return; }

    // once more, since a state may have changed the size of an element
    Arrange();

    const auto [width, height] = _renderer->GetRenderResolution();
    _painter.Begin(width, height);

    for (const auto &document : _documents) { document->root->Paint(_painter, document->frame, 1.0f); }

    _painter.End();
    _draw_calls = _painter.GetDrawCalls();
  }
} // neon
