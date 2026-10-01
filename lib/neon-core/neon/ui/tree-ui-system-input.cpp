#include "tree-ui-system.hpp"

#include <algorithm>
#include <cmath>

// What the pointer, the keys, and a controller do to a user interface, and
// what of them is left for the game.

namespace neon
{
  namespace
  {
    // seconds and pixels within which a second press counts as one more of
    // the same
    constexpr double click_time = 0.4;
    constexpr float click_distance = 6.0f;

    // seconds the pointer rests before what it rests on says what it is
    constexpr float tooltip_delay = 0.6f;

    // how far the pointer moves before what it holds is dragged
    constexpr float drag_threshold = 6.0f;

    bool IsInside(const UiElement *element, const UiElement *other)
    {
      for (const UiElement *above = element; above != nullptr; above = above->GetParent())
      {
        if (above == other) { return true; }
      }
      return false;
    }

    Key KeyOf(const int direction)
    {
      switch (direction)
      {
        case 0: return Key::Up;
        case 1: return Key::Right;
        case 2: return Key::Down;
        default: return Key::Left;
      }
    }

    CursorShape ShapeOf(const UiCursor cursor)
    {
      switch (cursor)
      {
        case UiCursor::Pointer: return CursorShape::Pointer;
        case UiCursor::Text: return CursorShape::Text;
        case UiCursor::Wait: return CursorShape::Wait;
        case UiCursor::Progress: return CursorShape::Progress;
        case UiCursor::Crosshair: return CursorShape::Crosshair;
        case UiCursor::Move: return CursorShape::Move;
        case UiCursor::NotAllowed: return CursorShape::NotAllowed;
        case UiCursor::EwResize: return CursorShape::EwResize;
        case UiCursor::NsResize: return CursorShape::NsResize;
        case UiCursor::NeswResize: return CursorShape::NeswResize;
        case UiCursor::NwseResize: return CursorShape::NwseResize;
        case UiCursor::Grab: return CursorShape::Grab;
        case UiCursor::Grabbing: return CursorShape::Grabbing;
        case UiCursor::None: return CursorShape::None;
        default: return CursorShape::Default;
      }
    }

    bool IsDraggable(const UiElement &element)
    {
      std::string value;
      return element.GetCssAttribute("draggable", value) && value == "true";
    }
  }

  bool Tree_UiSystem::Interact(UiElement &element, UiInteraction &interaction)
  {
    const UiDocument *document = DocumentOf(&element);
    if (document == nullptr) { return false; }

    interaction.is_used = false;
    interaction.clipboard = _clipboard;

    element.Interact(interaction, document->frame);
    return interaction.is_used;
  }

  void Tree_UiSystem::SetFocus(UiElement *element, const bool by_keys)
  {
    if (element == _focused) { return; }

    _focused = element;
    _focus_came_by_keys = by_keys;
  }

  void Tree_UiSystem::FollowFocus()
  {
    const std::uint64_t now = _focused != nullptr ? _focused->GetId() : 0;

    if (now != _told_focus)
    {
      UiElement *before = ElementOf(UiHandle{_told_focus});

      if (before != nullptr)
      {
        UiInteraction blur;
        blur.kind = UiInteraction::Kind::Blur;
        Interact(*before, blur);

        UiElementEvent event;
        event.related = UiHandle{now};
        Emit(*before, "blurred", event);
      }

      if (_focused != nullptr)
      {
        UiInteraction focus;
        focus.kind = UiInteraction::Kind::Focus;
        Interact(*_focused, focus);

        UiElementEvent event;
        event.related = UiHandle{before != nullptr ? _told_focus : 0};
        Emit(*_focused, "focused", event);

        // what the keys moved the focus to may be out of sight
        if (_focus_came_by_keys) { ScrollToShow(*_focused); }
      }

      _told_focus = now;
    }

    // The platform is asked for text while something has the focus that a
    // text is typed into, and told where the caret is, so that an input
    // method shows what it offers next to it.
    const bool types = _focused != nullptr && _focused->TakesText() && _focused->IsEnabled();

    if (!types)
    {
      if (_is_typing) { _input->StopTextInput(); }
      _is_typing = false;
      return;
    }

    const UiDocument *document = DocumentOf(_focused);
    const float scale = ScaleOf(_focused);

    UiRectangle caret = _focused->GetBox();
    if (document != nullptr) { (void) _focused->GetCaretBox(document->frame, caret); }

    const UiRectangle pixels = ToPixels(caret, scale);
    const TextInputArea area{
      static_cast<int>(pixels.left),
      static_cast<int>(pixels.top),
      std::max(1, static_cast<int>(pixels.Width())),
      std::max(1, static_cast<int>(pixels.Height()))
    };

    if (!_is_typing || !(area == _caret_area))
    {
      _input->StartTextInput(area);
      _caret_area = area;
    }

    _is_typing = true;
  }

  void Tree_UiSystem::FollowHover(UiElement *hovered)
  {
    if (hovered == _hovered) { return; }

    UiElementEvent event;
    event.x = _pointer_x / ScaleOf(hovered != nullptr ? hovered : _hovered);
    event.y = _pointer_y / ScaleOf(hovered != nullptr ? hovered : _hovered);

    // what the pointer left, from the inside out, and what it entered,
    // from the outside in
    for (UiElement *each = _hovered; each != nullptr; each = each->GetParent())
    {
      if (hovered == nullptr || !IsInside(hovered, each)) { Emit(*each, "pointer_leave", event); }
    }

    std::vector<UiElement *> entered;
    for (UiElement *each = hovered; each != nullptr; each = each->GetParent())
    {
      if (_hovered == nullptr || !IsInside(_hovered, each)) { entered.push_back(each); }
    }

    for (std::size_t i = entered.size(); i > 0; i--) { Emit(*entered[i - 1], "pointer_enter", event); }
  }

  void Tree_UiSystem::HandlePointer(const InputState &input, UiElement *hovered, UiConsumed &consumed)
  {
    const bool has_pointer = input.HasPointer();
    const bool pointer_is_down = has_pointer && input[Action::Pointer_Primary];
    const bool went_down = pointer_is_down && !_pointer_was_down;
    const bool went_up = !pointer_is_down && _pointer_was_down;

    if (has_pointer)
    {
      _pointer_x = static_cast<float>(input.GetPointer().x);
      _pointer_y = static_cast<float>(input.GetPointer().y);
    }

    const auto in_units = [this](const UiElement *element, UiInteraction &interaction)
    {
      const float scale = ScaleOf(element);
      interaction.x = _pointer_x / scale;
      interaction.y = _pointer_y / scale;
    };

    const auto event_at = [this](const UiElement *element)
    {
      UiElementEvent event;
      const float scale = ScaleOf(element);
      event.x = _pointer_x / scale;
      event.y = _pointer_y / scale;
      event.clicks = _press_count;
      return event;
    };

    bool pressed_scrollbar = false;

    if (went_down)
    {
      // how often in a row, in one place and without a pause
      const bool is_again = _press_time >= 0.0 && _time - _press_time <= click_time &&
                            std::abs(_pointer_x - _press_x) <= click_distance &&
                            std::abs(_pointer_y - _press_y) <= click_distance &&
                            _press_element == (hovered != nullptr ? hovered->GetId() : 0);

      _press_count = is_again ? _press_count + 1 : 1;
      _press_time = _time;
      _press_x = _pointer_x;
      _press_y = _pointer_y;
      _press_element = hovered != nullptr ? hovered->GetId() : 0;

      pressed_scrollbar = PressScrollbar(_pointer_x, _pointer_y);

      if (pressed_scrollbar)
      {
        _pressed = nullptr;
        _captured = nullptr;
      } else
      {
        const bool can_be_used = hovered != nullptr && hovered->IsEnabled();

        _pressed = can_be_used && hovered->IsClickable() ? hovered : nullptr;

        // a press next to what has something open closes it
        if (_focused != nullptr && _focused->HasTopLayer() && hovered != _focused)
        {
          UiInteraction cancel;
          cancel.kind = UiInteraction::Kind::Cancel;
          Interact(*_focused, cancel);
        }

        if (can_be_used && hovered->IsFocusable())
        {
          SetFocus(hovered, false);
        } else if (_focused != nullptr && _focused->TakesText())
        {
          // a press next to a text that is typed ends the typing
          SetFocus(nullptr, false);
        }

        _captured = nullptr;

        if (can_be_used)
        {
          UiInteraction interaction;
          interaction.kind = UiInteraction::Kind::PointerDown;
          interaction.clicks = _press_count;
          interaction.modifiers = input.GetKeyEvents().empty() ? KeyModifiers{} : input.GetKeyEvents().back().modifiers;
          in_units(hovered, interaction);

          if (Interact(*hovered, interaction)) { _captured = hovered; }
        }

        _drag_candidate = hovered != nullptr && IsDraggable(*hovered) ? hovered->GetId() : 0;

        if (hovered != nullptr) { Emit(*hovered, "pointer_down", event_at(hovered)); }
      }
    }

    if (pointer_is_down && _thumb_drag.element != 0) { DragScrollbar(_pointer_x, _pointer_y); }

    DragContent(input, pointer_is_down, went_down && !pressed_scrollbar, _frame_seconds);

    if (pointer_is_down && _captured != nullptr && !went_down)
    {
      UiInteraction interaction;
      interaction.kind = UiInteraction::Kind::PointerMove;
      in_units(_captured, interaction);
      Interact(*_captured, interaction);
    }

    // dragging an element onto another
    if (pointer_is_down && _drag_candidate != 0 && _dragged == 0)
    {
      const float moved = std::max(std::abs(_pointer_x - _press_x), std::abs(_pointer_y - _press_y));

      if (UiElement *candidate = ElementOf(UiHandle{_drag_candidate});
        candidate != nullptr && moved >= drag_threshold)
      {
        _dragged = _drag_candidate;
        _drag_over = 0;

        // what is dragged is not clicked
        _pressed = nullptr;

        Emit(*candidate, "drag_start", event_at(candidate));
      }
    }

    if (pointer_is_down && _dragged != 0)
    {
      // what is under the pointer, which may be what takes no pointer
      UiElement *under = hovered != nullptr ? hovered : ElementAt(_pointer_x, _pointer_y);
      const std::uint64_t over = under != nullptr ? under->GetId() : 0;

      if (under != nullptr && over != _dragged)
      {
        UiElementEvent event = event_at(under);
        event.related = UiHandle{_dragged};

        // told while it is over something, and not only when it gets there
        Emit(*under, "drag_over", event);
      }

      _drag_over = over;
    }

    consumed.pointer = hovered != nullptr || _pressed != nullptr || _captured != nullptr ||
                       _thumb_drag.element != 0 || _content_drag.is_dragging || _dragged != 0 ||
                       pressed_scrollbar;

    if (went_up)
    {
      if (_captured != nullptr)
      {
        UiInteraction interaction;
        interaction.kind = UiInteraction::Kind::PointerUp;
        in_units(_captured, interaction);
        Interact(*_captured, interaction);
      }

      if (hovered != nullptr) { Emit(*hovered, "pointer_up", event_at(hovered)); }

      if (_dragged != 0)
      {
        UiElement *under = hovered != nullptr ? hovered : ElementAt(_pointer_x, _pointer_y);

        if (under != nullptr && under->GetId() != _dragged)
        {
          UiElementEvent event = event_at(under);
          event.related = UiHandle{_dragged};
          Emit(*under, "drop", event);
        }

        if (UiElement *dragged = ElementOf(UiHandle{_dragged}); dragged != nullptr)
        {
          UiElementEvent event = event_at(dragged);
          event.related = UiHandle{under != nullptr ? under->GetId() : 0};
          Emit(*dragged, "drag_end", event);
        }
      } else if (_pressed != nullptr && hovered == _pressed)
      {
        // a click is a press and a release on the same element
        Click(*_pressed, true);

        if (_press_count == 2) { Emit(*_pressed, "double_click", event_at(_pressed)); }
      }

      _pressed = nullptr;
      _captured = nullptr;
      _thumb_drag = {};
      _drag_candidate = 0;
      _dragged = 0;
      _drag_over = 0;
    }

    _pointer_was_down = pointer_is_down;
  }

  void Tree_UiSystem::MoveFocusInOrder(const bool backwards)
  {
    std::vector<UiElement *> in_order;
    std::vector<UiElement *> first;

    for (std::size_t i = FirstActive(); i < _documents.size(); i++)
    {
      std::vector<UiElement *> elements;
      CollectUiElements(*_documents[i]->root, elements);

      for (UiElement *element : elements)
      {
        if (!element->IsFocusable() || !CanBeUsed(element) || element->GetTabIndex() < 0) { continue; }

        if (element->GetTabIndex() > 0)
        {
          first.push_back(element);
        } else
        {
          in_order.push_back(element);
        }
      }
    }

    // As in HTML: what has a place of its own comes first, the lowest in
    // front, and then everything else as the file has it.
    std::ranges::stable_sort(first, [](const UiElement *a, const UiElement *b)
    {
      return a->GetTabIndex() < b->GetTabIndex();
    });

    first.insert(first.end(), in_order.begin(), in_order.end());
    if (first.empty()) { return; }

    const auto found = std::ranges::find(first, _focused);

    if (found == first.end())
    {
      SetFocus(backwards ? first.back() : first.front(), true);
      return;
    }

    const auto place = static_cast<std::size_t>(found - first.begin());
    const std::size_t next = backwards
      ? (place + first.size() - 1) % first.size()
      : (place + 1) % first.size();

    SetFocus(first[next], true);
  }

  void Tree_UiSystem::Cancel()
  {
    // what has something open closes it, and that is all
    if (_focused != nullptr)
    {
      UiInteraction interaction;
      interaction.kind = UiInteraction::Kind::Cancel;
      if (Interact(*_focused, interaction)) { return; }
    }

    if (_documents.empty()) { return; }

    // the file that takes the input: that of what has the focus, or the
    // one on top
    UiDocument *document = DocumentOf(_focused);
    if (document == nullptr) { document = _documents.back().get(); }

    // the game is told in every case, and may do what the file does not
    Emit(_focused != nullptr ? *_focused : *document->root, "cancel");

    UiCancel cancel = document->cancel;
    if (cancel == UiCancel::Auto) { cancel = document->modal ? UiCancel::Close : UiCancel::Blur; }

    switch (cancel)
    {
      case UiCancel::Close:
        _documents_to_close.push_back(document->id);
        break;
      case UiCancel::Blur:
        SetFocus(nullptr, false);
        break;
      default:
        break;
    }
  }

  void Tree_UiSystem::HandleKeys(const InputState &input, UiConsumed &consumed)
  {
    const bool types = _focused != nullptr && _focused->TakesText() && _focused->IsEnabled();

    for (int i = 0; i < 4; i++) { _direction_was_refused[i] = false; }

    // what is typed, and what an input method is putting together
    if (types)
    {
      consumed.keyboard = true;

      if (!(input.GetComposition() == _composition))
      {
        _composition = input.GetComposition();

        UiInteraction interaction;
        interaction.kind = UiInteraction::Kind::Composition;
        interaction.text = _composition.text;
        interaction.composition_cursor = _composition.cursor;
        Interact(*_focused, interaction);
      }

      if (!input.GetText().empty())
      {
        UiInteraction interaction;
        interaction.kind = UiInteraction::Kind::Text;
        interaction.text = input.GetText();
        Interact(*_focused, interaction);
      }
    } else
    {
      _composition = {};
    }

    for (const auto &key : input.GetKeyEvents())
    {
      UiElement *focused = _focused;

      if (focused != nullptr)
      {
        UiElementEvent event;
        event.key = key.key;
        event.modifiers = key.modifiers;
        event.value = NameOf(key.key);
        Emit(*focused, key.is_down ? "key_down" : "key_up", event);
      }

      if (!key.is_down) { continue; }

      if (focused != nullptr && focused->IsEnabled())
      {
        UiInteraction interaction;
        interaction.kind = UiInteraction::Kind::KeyDown;
        interaction.key = key.key;
        interaction.modifiers = key.modifiers;

        if (Interact(*focused, interaction)) { continue; }
      }

      if (key.key == Key::Tab && !key.modifiers.shortcut && !key.modifiers.alt)
      {
        MoveFocusInOrder(key.modifiers.shift);
        consumed.keyboard = true;
      }

      // while a text is typed, the keys that cancel are its own, but for
      // escape, which cancels in every case
      if (key.key == Key::Escape && types)
      {
        Cancel();
        consumed.keyboard = true;
      }
    }

    // the directions, of the keys and of a controller
    constexpr Action directions[4] = {Action::Ui_Up, Action::Ui_Right, Action::Ui_Down, Action::Ui_Left};

    for (int i = 0; i < 4; i++)
    {
      // while a text is typed, the arrows of the keyboard belong to it
      const bool is_down = types
        ? input.IsHeldByOtherThanKeyboard(directions[i])
        : static_cast<bool>(input[directions[i]]);

      if (is_down && !_direction_was_down[i])
      {
        const Key key = KeyOf(i);

        if (_focused != nullptr && _focused->IsEnabled() && _focused->UsesDirection(key))
        {
          // what has the focus moves by itself: a slider, and what
          // chooses one of several
          UiInteraction interaction;
          interaction.kind = UiInteraction::Kind::Direction;
          interaction.key = key;
          Interact(*_focused, interaction);
        } else if (!MoveFocus(static_cast<Direction>(i)))
        {
          _direction_was_refused[i] = true;
        }
      }

      _direction_was_down[i] = is_down;
    }

    const bool accept_is_down = types
      ? input.IsHeldByOtherThanKeyboard(Action::Ui_Accept)
      : static_cast<bool>(input[Action::Ui_Accept]);

    if (accept_is_down && !_accept_was_down && CanBeUsed(_focused) && _focused->IsClickable())
    {
      Click(*_focused, false);
    }
    _accept_was_down = accept_is_down;

    // backspace cancels, and deletes while a text is typed
    const bool cancel_is_down = types
      ? input.IsHeldByOtherThanKeyboard(Action::Ui_Cancel)
      : static_cast<bool>(input[Action::Ui_Cancel]);

    if (cancel_is_down && !_cancel_was_down) { Cancel(); }
    _cancel_was_down = cancel_is_down;
  }

  void Tree_UiSystem::FollowTooltip(UiElement *at, const float seconds)
  {
    // the nearest element, from the one under the pointer upwards, that
    // says what it is
    UiElement *titled = at;
    while (titled != nullptr && titled->GetTitle().empty()) { titled = titled->GetParent(); }

    const std::uint64_t id = titled != nullptr ? titled->GetId() : 0;

    if (id != _tooltip.element || _pointer_was_down)
    {
      if (_tooltip.is_shown) { _needs_paint = true; }

      _tooltip = {};
      _tooltip.element = _pointer_was_down ? 0 : id;
      return;
    }

    if (titled == nullptr || _tooltip.is_shown) { return; }

    _tooltip.rested += seconds;
    if (_tooltip.rested < tooltip_delay) { return; }

    _tooltip.is_shown = true;
    _tooltip.text = titled->GetTitle();
    _tooltip.x = _pointer_x;
    _tooltip.y = _pointer_y;
    _needs_paint = true;
  }

  void Tree_UiSystem::FollowCursor(const UiElement *hovered)
  {
    CursorShape shape = CursorShape::Default;

    if (_dragged != 0)
    {
      shape = CursorShape::Grabbing;
    } else if (hovered != nullptr)
    {
      const UiCursor cursor = hovered->GetStyle().cursor;

      if (cursor != UiCursor::Auto)
      {
        shape = ShapeOf(cursor);
      } else if (hovered->TakesText() && hovered->IsEnabled())
      {
        // what a browser does for `auto`
        shape = CursorShape::Text;
      }
    }

    if (shape == _cursor_shape) { return; }

    _cursor_shape = shape;
    if (_window != nullptr) { _window->SetCursorShape(shape); }
  }

  void Tree_UiSystem::TickElements(const float seconds)
  {
    for (const auto &document : _documents)
    {
      if (document->ticking.empty()) { continue; }

      std::vector<UiElement *> elements;
      elements.swap(document->ticking);

      for (UiElement *element : elements)
      {
        element->ClearDirty(UiDirty::Tick);

        if (element->Tick(document->frame, seconds) &&
            std::ranges::find(document->ticking, element) == document->ticking.end())
        {
          document->ticking.push_back(element);
        }
      }
    }
  }

  void Tree_UiSystem::Update()
  {
    _events.clear();
    _element_events.clear();
    _resources.BeginFrame();

    const InputState &input = _input->GetInputState();
    const bool modal = HasModal();

    // What the player used last, for the hints a file shows: a controller
    // is not told to click. Said as values every file sees.
    const bool uses_gamepad = input.GetDevice() == InputDevice::Gamepad;
    if (!_has_device || uses_gamepad != _uses_gamepad)
    {
      _has_device = true;
      _uses_gamepad = uses_gamepad;
      _values.Set("input_device", UiValue::Text(uses_gamepad ? "gamepad" : "keyboard"));
      _values.Set("gamepad", UiValue::Flag(uses_gamepad));
    }

    if (_documents.empty())
    {
      _gate.SetNeedsPointer(false);
      _gate.Refresh({});
      _pointer_was_down = input[Action::Pointer_Primary];
      _accept_was_down = input[Action::Ui_Accept];
      _cancel_was_down = input[Action::Ui_Cancel];

      for (const auto &surface : _surfaces)
      {
        if (surface != nullptr) { surface->pointer_was_down = surface->pointer_is_down; }
      }
      _surfaces[Ui_Window_Surface]->pointer_was_down = _pointer_was_down;

      if (_is_typing) { _input->StopTextInput(); }
      _is_typing = false;
      return;
    }

    // the cursor is shown before the pointer is asked for, unless the
    // player holds a controller, which has no use for it
    _gate.SetNeedsPointer(modal && !uses_gamepad);

    // Time is what the user interface was told, and what the window says
    // without that. It runs by itself, whatever the world does.
    double seconds = _was_advanced ? _advance : 0.0;
    if (!_was_advanced && _window != nullptr) { seconds = _window->GetDeltaTime(); }

    seconds *= _time_scale;
    _time += seconds;
    _frame_seconds = static_cast<float>(seconds);
    _was_advanced = false;
    _advance = 0.0;

    Settle();

    // what can no longer be used loses the focus, and so does what is on a
    // surface that does not have the keys
    if (_focused != nullptr && (!CanBeUsed(_focused) || !TakesInput(_focused))) { SetFocus(nullptr, false); }
    if (_pressed != nullptr && (!CanBeUsed(_pressed) || !TakesInput(_pressed))) { _pressed = nullptr; }
    if (_captured != nullptr && (!CanBeUsed(_captured) || !TakesInput(_captured))) { _captured = nullptr; }

    if (const UiDocument *document = DocumentOf(_focused);
      document != nullptr && document->surface != _input_surface)
    {
      SetFocus(nullptr, false);
    }

    UiConsumed consumed;
    consumed.everything = modal;

    // The pointer of the window, which comes from the input.
    UiElement *hovered = nullptr;
    if (input.HasPointer())
    {
      hovered = HitTest(static_cast<float>(input.GetPointer().x), static_cast<float>(input.GetPointer().y));
    }

    // what the pointer came to is told first, and then what it did there
    if (input.HasPointer())
    {
      _pointer_x = static_cast<float>(input.GetPointer().x);
      _pointer_y = static_cast<float>(input.GetPointer().y);
    }

    FollowHover(hovered);
    HandlePointer(input, hovered, consumed);

    Surface &window = *_surfaces[Ui_Window_Surface];
    window.hovered = hovered;
    window.pressed = _pressed;
    window.pointer_was_down = _pointer_was_down;
    window.uses_pointer = consumed.pointer;

    // The pointer of every other surface, which whoever knows where the
    // player points on it has said.
    for (const auto &surface : _surfaces)
    {
      if (surface == nullptr || surface->id == Ui_Window_Surface) { continue; }

      UpdatePointer(
        *surface, surface->has_pointer, surface->pointer_x, surface->pointer_y, surface->pointer_is_down);
    }

    // the keys and the controller, which are the user interface's while
    // something has the focus, and when they take it away
    const bool had_focus = _focused != nullptr;
    HandleKeys(input, consumed);

    ScrollWithWheel(input, consumed);
    ScrollWithKeys(input);
    ScrollWithStick(input, _frame_seconds, consumed);
    MoveScrolling(_frame_seconds);

    consumed.navigation = had_focus || _focused != nullptr;

    FollowFocus();
    TickElements(_frame_seconds);

    const bool accept_is_down = _accept_was_down;
    UpdateStates(accept_is_down);

    FollowTooltip(
      input.HasPointer() ? ElementAt(_pointer_x, _pointer_y) : nullptr,
      _frame_seconds);
    FollowCursor(hovered);

    Animate(_frame_seconds);
    CollectNotices();

    _gate.Refresh(consumed);

    // Called last, and from a copy. A callback is free to load and unload
    // files, and to cause events of its own.
    const std::vector<UiEvent> events = _events;
    for (const auto &event : events)
    {
      // the one for the element of one user interface, where there is one
      if (const auto found = _callbacks_in.find({event.document, event.element}); found != _callbacks_in.end())
      {
        const std::function<void()> callback = found->second;
        callback();
        continue;
      }

      if (const auto found = _callbacks.find(event.element); found != _callbacks.end())
      {
        const std::function<void()> callback = found->second;
        callback();
      }
    }

    DispatchEvents();

    // what cancel closed, behind everything that was told of it
    const std::vector<int> to_close = _documents_to_close;
    _documents_to_close.clear();
    for (const int document : to_close) { Unload(document); }
  }
} // neon
