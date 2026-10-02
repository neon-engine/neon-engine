#include "tree-ui-system.hpp"

#include <algorithm>
#include <cmath>

// Scrolling: by the wheel, by the scrollbars, by keys, by the right stick
// of a controller, by dragging what is inside, and to what has the focus.

namespace neon
{
  // Helpers of tree-ui-system-scroll.cpp, for this file alone.
  namespace
  {
    // how far a notch of the wheel scrolls, in units of the file: three
    // lines of text at the size a file starts with
    constexpr float notch = 60.0f;

    // how far an arrow scrolls, and how much of a page a page is
    constexpr float step = 40.0f;
    constexpr float page = 0.9f;

    // how far the stick scrolls when it is pushed all the way, in units
    // for each second
    constexpr float stick_speed = 1200.0f;

    // the stick is handed over as it is, and rests a little off the middle
    constexpr double stick_rest = 0.25;

    // seconds that scrolling to a place takes
    constexpr float move_time = 0.25f;

    // how far the pointer moves before what is inside is dragged
    constexpr float drag_threshold = 6.0f;

    // how fast what was let go slows down, and where it stops
    constexpr float friction = 4.0f;
    constexpr float least_speed = 8.0f;

    float Along(const UiRectangle &rectangle, const UiScrollbars::Axis axis)
    {
      return axis == UiScrollbars::Axis::Horizontal ? rectangle.left : rectangle.top;
    }

    bool Contains(const UiRectangle &rectangle, const float x, const float y)
    {
      return !rectangle.IsEmpty() && rectangle.Contains(x, y);
    }
  }

  float Tree_UiSystem::ScaleOf(const UiElement *element) const
  {
    const UiDocument *document = DocumentOf(element);
    return document != nullptr && document->frame.scale > 0.0f ? document->frame.scale : 1.0f;
  }

  UiElement *Tree_UiSystem::ElementAt(UiElement &element, const float scale, const float x, const float y)
  {
    if (element.IsHidden() || element.IsClippedAway()) { return nullptr; }

    const UiStyle &style = element.GetStyle();
    const UiRectangle padding_box = ToPixels(element.GetPaddingBox(), scale);

    const bool children_can_be_hit =
      (!style.ClipsX() || (x >= padding_box.left && x < padding_box.right)) &&
      (!style.ClipsY() || (y >= padding_box.top && y < padding_box.bottom));

    if (children_can_be_hit)
    {
      const auto children = element.GetChildrenInPaintOrder();
      for (std::size_t i = children.size(); i > 0; i--)
      {
        if (UiElement *hit = ElementAt(*children[i - 1], scale, x, y); hit != nullptr) { return hit; }
      }
    }

    return ToPixels(element.GetVisibleBox(), scale).Contains(x, y) ? &element : nullptr;
  }

  UiElement *Tree_UiSystem::ElementAt(const float x, const float y) const
  {
    if (_documents.empty()) { return nullptr; }

    const std::size_t first = FirstActive();
    for (std::size_t i = _documents.size(); i > first; i--)
    {
      const UiDocument &document = *_documents[i - 1];
      if (UiElement *hit = ElementAt(*document.root, document.frame.scale, x, y); hit != nullptr) { return hit; }
    }
    return nullptr;
  }

  UiElement *Tree_UiSystem::ScrollerOf(UiElement *from, const float by_x, const float by_y)
  {
    for (UiElement *element = from; element != nullptr; element = element->GetParent())
    {
      if (element->IsHidden()) { continue; }

      const bool can_x = by_x != 0.0f && element->CanScrollX() &&
                         (by_x > 0.0f ? element->GetScrollX() < element->GetMaxScrollX() : element->GetScrollX() > 0.0f);
      const bool can_y = by_y != 0.0f && element->CanScrollY() &&
                         (by_y > 0.0f ? element->GetScrollY() < element->GetMaxScrollY() : element->GetScrollY() > 0.0f);

      if (can_x || can_y) { return element; }
    }

    return nullptr;
  }

  bool Tree_UiSystem::ScrollTo(UiElement &element, const float x, const float y, const bool at_once)
  {
    const float to_x = std::clamp(x, 0.0f, element.GetMaxScrollX());
    const float to_y = std::clamp(y, 0.0f, element.GetMaxScrollY());

    const std::uint64_t id = element.GetId();
    std::erase_if(_scroll_moves, [id](const ScrollMove &move) { return move.element == id; });
    std::erase_if(_glides, [id](const Glide &glide) { return glide.element == id; });

    if (to_x == element.GetScrollX() && to_y == element.GetScrollY()) { return false; }

    const bool takes_time = !at_once && !_reduced_motion &&
                            element.GetStyle().scroll_behavior == UiScrollBehavior::Smooth;

    if (takes_time)
    {
      _scroll_moves.push_back({id, element.GetScrollX(), element.GetScrollY(), to_x, to_y, 0.0f});
      return true;
    }

    if (element.SetScroll(to_x, to_y)) { Emit(element, "scroll"); }
    return true;
  }

  void Tree_UiSystem::ScrollToShow(UiElement &element)
  {
    UiRectangle box = element.GetBox();

    for (UiElement *around = element.GetParent(); around != nullptr; around = around->GetParent())
    {
      const UiStyle &style = around->GetStyle();
      if (!style.ScrollsX() && !style.ScrollsY()) { continue; }

      // where it is scrolled to, which is where it is on its way to
      float scroll_x = around->GetScrollX();
      float scroll_y = around->GetScrollY();

      for (const auto &move : _scroll_moves)
      {
        if (move.element != around->GetId()) { continue; }

        // the box is where it was placed, which was at what is scrolled
        // by now
        box.left -= move.to_x - scroll_x;
        box.right -= move.to_x - scroll_x;
        box.top -= move.to_y - scroll_y;
        box.bottom -= move.to_y - scroll_y;

        scroll_x = move.to_x;
        scroll_y = move.to_y;
      }

      const UiRectangle room = around->GetPaddingBox();

      // As little as it takes. What is larger than the room shows its
      // start.
      const auto needed = [](const float start, const float end, const float room_start, const float room_end)
      {
        if (start < room_start) { return start - room_start; }
        if (end > room_end) { return std::min(end - room_end, start - room_start); }
        return 0.0f;
      };

      const float by_x = style.ScrollsX() ? needed(box.left, box.right, room.left, room.right) : 0.0f;
      const float by_y = style.ScrollsY() ? needed(box.top, box.bottom, room.top, room.bottom) : 0.0f;

      if (by_x == 0.0f && by_y == 0.0f) { continue; }

      const float to_x = std::clamp(scroll_x + by_x, 0.0f, around->GetMaxScrollX());
      const float to_y = std::clamp(scroll_y + by_y, 0.0f, around->GetMaxScrollY());

      ScrollTo(*around, to_x, to_y, false);

      // what is around the one that was scrolled finds the element where
      // it is afterwards
      box.left -= to_x - scroll_x;
      box.right -= to_x - scroll_x;
      box.top -= to_y - scroll_y;
      box.bottom -= to_y - scroll_y;
    }
  }

  bool Tree_UiSystem::PressScrollbar(const float x, const float y)
  {
    for (UiElement *element = ElementAt(x, y); element != nullptr; element = element->GetParent())
    {
      const float scale = ScaleOf(element);
      const float at_x = x / scale;
      const float at_y = y / scale;

      for (const auto axis : {UiScrollbars::Axis::Vertical, UiScrollbars::Axis::Horizontal})
      {
        const UiRectangle track = UiScrollbars::TrackOf(*element, axis);
        if (!Contains(track, at_x, at_y)) { continue; }

        const UiRectangle thumb = UiScrollbars::ThumbOf(*element, axis);
        const float along = axis == UiScrollbars::Axis::Horizontal ? at_x : at_y;

        if (Contains(thumb, at_x, at_y))
        {
          _thumb_drag = {element->GetId(), axis, along - Along(thumb, axis)};
          return true;
        }

        // next to the thumb: a page towards where the pointer is
        const bool is_behind = along > Along(thumb, axis);
        const UiRectangle room = element->GetPaddingBox();

        float to_x = element->GetScrollX();
        float to_y = element->GetScrollY();

        if (axis == UiScrollbars::Axis::Horizontal)
        {
          to_x += (is_behind ? 1.0f : -1.0f) * room.Width() * page;
        } else
        {
          to_y += (is_behind ? 1.0f : -1.0f) * room.Height() * page;
        }

        ScrollTo(*element, to_x, to_y, false);

        // the press went to the scrollbar, though nothing is dragged
        _thumb_drag = {};
        return true;
      }
    }

    return false;
  }

  void Tree_UiSystem::DragScrollbar(const float x, const float y)
  {
    UiElement *element = ElementOf(UiHandle{_thumb_drag.element});
    if (element == nullptr)
    {
      _thumb_drag = {};
      return;
    }

    const float scale = ScaleOf(element);
    const UiRectangle track = UiScrollbars::TrackOf(*element, _thumb_drag.axis);

    const float along = (_thumb_drag.axis == UiScrollbars::Axis::Horizontal ? x : y) / scale;
    const float scrolled = UiScrollbars::ScrollFor(
      *element, _thumb_drag.axis, along - _thumb_drag.grip - Along(track, _thumb_drag.axis));

    if (_thumb_drag.axis == UiScrollbars::Axis::Horizontal)
    {
      ScrollTo(*element, scrolled, element->GetScrollY(), true);
    } else
    {
      ScrollTo(*element, element->GetScrollX(), scrolled, true);
    }
  }

  void Tree_UiSystem::ScrollWithWheel(const InputState &input, UiConsumed &consumed)
  {
    const AxisState &wheel = input.GetWheel();
    if ((wheel.x == 0.0 && wheel.y == 0.0) || !input.HasPointer()) { return; }

    UiElement *under = ElementAt(static_cast<float>(input.GetPointer().x), static_cast<float>(input.GetPointer().y));
    if (under == nullptr) { return; }

    float by_x = static_cast<float>(wheel.x) * notch;
    float by_y = static_cast<float>(wheel.y) * notch;

    // what the wheel is turned over hears of it
    UiElementEvent event;
    event.x = static_cast<float>(input.GetPointer().x) / ScaleOf(under);
    event.y = static_cast<float>(input.GetPointer().y) / ScaleOf(under);
    event.wheel_x = by_x;
    event.wheel_y = by_y;
    Emit(*under, "wheel", event);

    UiInteraction interaction;
    interaction.kind = UiInteraction::Kind::Wheel;
    interaction.x = event.x;
    interaction.y = event.y;
    interaction.wheel_x = by_x;
    interaction.wheel_y = by_y;

    for (UiElement *each = under; each != nullptr; each = each->GetParent())
    {
      if (each->IsEnabled() && Interact(*each, interaction))
      {
        consumed.wheel = true;
        return;
      }
    }

    // The innermost that has something left to scroll to takes the wheel.
    UiElement *scroller = ScrollerOf(under, by_x, by_y);

    // a wheel that only turns one way scrolls what only scrolls the other
    if (scroller == nullptr && by_x == 0.0f)
    {
      scroller = ScrollerOf(under, by_y, 0.0f);
      if (scroller != nullptr && !scroller->CanScrollY())
      {
        by_x = by_y;
        by_y = 0.0f;
      } else
      {
        scroller = nullptr;
      }
    }

    // Over something that scrolls, the wheel is the user interface's,
    // whether there is something left to scroll to or not.
    for (const UiElement *each = under; each != nullptr; each = each->GetParent())
    {
      if (each->CanScrollX() || each->CanScrollY()) { consumed.wheel = true; }
    }

    if (scroller == nullptr) { return; }

    ScrollTo(*scroller, scroller->GetScrollX() + by_x, scroller->GetScrollY() + by_y, true);
  }

  void Tree_UiSystem::ScrollWithKeys(const InputState &input)
  {
    if (_focused == nullptr) { return; }

    const auto scroll_by = [this](const float by_x, const float by_y)
    {
      UiElement *scroller = ScrollerOf(_focused, by_x, by_y);
      if (scroller == nullptr) { return; }

      ScrollTo(*scroller, scroller->GetScrollX() + by_x, scroller->GetScrollY() + by_y, false);
    };

    // an arrow that found nothing to move the focus to
    if (_direction_was_refused[static_cast<int>(Direction::Up)]) { scroll_by(0.0f, -step); }
    if (_direction_was_refused[static_cast<int>(Direction::Down)]) { scroll_by(0.0f, step); }
    if (_direction_was_refused[static_cast<int>(Direction::Left)]) { scroll_by(-step, 0.0f); }
    if (_direction_was_refused[static_cast<int>(Direction::Right)]) { scroll_by(step, 0.0f); }

    // in a text, these keys move the caret
    if (_focused->TakesText()) { return; }

    for (const auto &event : input.GetKeyEvents())
    {
      if (!event.is_down) { continue; }

      // the nearest that scrolls at all, whichever way
      UiElement *scroller = nullptr;
      for (UiElement *each = _focused; each != nullptr && scroller == nullptr; each = each->GetParent())
      {
        if (each->CanScrollX() || each->CanScrollY()) { scroller = each; }
      }

      if (scroller == nullptr) { return; }

      const float height = scroller->GetPaddingBox().Height();

      switch (event.key)
      {
        case Key::PageDown:
          ScrollTo(*scroller, scroller->GetScrollX(), scroller->GetScrollY() + height * page, false);
          break;
        case Key::PageUp:
          ScrollTo(*scroller, scroller->GetScrollX(), scroller->GetScrollY() - height * page, false);
          break;
        case Key::Home:
          ScrollTo(*scroller, scroller->GetScrollX(), 0.0f, false);
          break;
        case Key::End:
          ScrollTo(*scroller, scroller->GetScrollX(), scroller->GetMaxScrollY(), false);
          break;
        default:
          break;
      }
    }
  }

  void Tree_UiSystem::ScrollWithStick(const InputState &input, const float seconds, UiConsumed &consumed)
  {
    const AxisState &stick = input.GetRightStick();
    if (std::abs(stick.x) < stick_rest && std::abs(stick.y) < stick_rest) { return; }

    // what has the focus, and without that what is under the pointer
    UiElement *from = _focused;
    if (from == nullptr && input.HasPointer())
    {
      from = ElementAt(static_cast<float>(input.GetPointer().x), static_cast<float>(input.GetPointer().y));
    }

    const float by_x = static_cast<float>(stick.x) * stick_speed * seconds;
    const float by_y = static_cast<float>(stick.y) * stick_speed * seconds;

    UiElement *scroller = ScrollerOf(from, by_x, by_y);

    for (const UiElement *each = from; each != nullptr; each = each->GetParent())
    {
      if (each->CanScrollX() || each->CanScrollY()) { consumed.right_stick = true; }
    }

    if (scroller == nullptr) { return; }

    ScrollTo(*scroller, scroller->GetScrollX() + by_x, scroller->GetScrollY() + by_y, true);
  }

  void Tree_UiSystem::DragContent(
    const InputState &input,
    const bool pointer_is_down,
    const bool pointer_went_down,
    const float seconds)
  {
    const float x = input.HasPointer() ? static_cast<float>(input.GetPointer().x) : 0.0f;
    const float y = input.HasPointer() ? static_cast<float>(input.GetPointer().y) : 0.0f;

    if (pointer_went_down && _thumb_drag.element == 0)
    {
      _content_drag = {};

      for (UiElement *each = ElementAt(x, y); each != nullptr; each = each->GetParent())
      {
        if (each->GetStyle().scroll_drag != UiScrollDrag::Inertia) { continue; }
        if (!each->CanScrollX() && !each->CanScrollY()) { continue; }

        const float scale = ScaleOf(each);

        _content_drag.element = each->GetId();
        _content_drag.start_x = x / scale;
        _content_drag.start_y = y / scale;
        _content_drag.last_x = _content_drag.start_x;
        _content_drag.last_y = _content_drag.start_y;
        _content_drag.scroll_x = each->GetScrollX();
        _content_drag.scroll_y = each->GetScrollY();

        // taking hold of what goes on by itself stops it
        const std::uint64_t id = each->GetId();
        std::erase_if(_glides, [id](const Glide &glide) { return glide.element == id; });
        break;
      }
    }

    if (_content_drag.element == 0) { return; }

    UiElement *element = ElementOf(UiHandle{_content_drag.element});
    if (element == nullptr)
    {
      _content_drag = {};
      return;
    }

    if (!pointer_is_down)
    {
      // let go: it goes on as fast as it was dragged
      if (_content_drag.is_dragging && !_reduced_motion &&
          (std::abs(_content_drag.speed_x) > least_speed || std::abs(_content_drag.speed_y) > least_speed))
      {
        _glides.push_back({element->GetId(), _content_drag.speed_x, _content_drag.speed_y});
      }

      _content_drag = {};
      return;
    }

    const float scale = ScaleOf(element);
    const float at_x = x / scale;
    const float at_y = y / scale;

    if (!_content_drag.is_dragging)
    {
      const float moved = std::max(
        std::abs(at_x - _content_drag.start_x), std::abs(at_y - _content_drag.start_y));
      if (moved < drag_threshold) { return; }

      _content_drag.is_dragging = true;

      // what is dragged is not clicked
      _pressed = nullptr;
      _captured = nullptr;
      _drag_candidate = 0;
    }

    ScrollTo(
      *element,
      _content_drag.scroll_x - (at_x - _content_drag.start_x),
      _content_drag.scroll_y - (at_y - _content_drag.start_y),
      true);

    if (seconds > 0.0f)
    {
      // how fast, evened out over a few frames
      const float now_x = (_content_drag.last_x - at_x) / seconds;
      const float now_y = (_content_drag.last_y - at_y) / seconds;

      _content_drag.speed_x = _content_drag.speed_x * 0.6f + now_x * 0.4f;
      _content_drag.speed_y = _content_drag.speed_y * 0.6f + now_y * 0.4f;
    }

    _content_drag.last_x = at_x;
    _content_drag.last_y = at_y;
  }

  void Tree_UiSystem::MoveScrolling(const float seconds)
  {
    if (seconds <= 0.0f) { return; }

    for (auto &move : _scroll_moves)
    {
      UiElement *element = ElementOf(UiHandle{move.element});
      if (element == nullptr)
      {
        move.elapsed = move_time;
        continue;
      }

      move.elapsed = std::min(move_time, move.elapsed + seconds);

      // fast at first and slow at the end
      const float progress = UiTimingFunction::EaseOut().At(move.elapsed / move_time);

      const float x = move.from_x + (move.to_x - move.from_x) * progress;
      const float y = move.from_y + (move.to_y - move.from_y) * progress;

      if (element->SetScroll(move.elapsed >= move_time ? move.to_x : x, move.elapsed >= move_time ? move.to_y : y))
      {
        Emit(*element, "scroll");
      }
    }

    std::erase_if(_scroll_moves, [](const ScrollMove &move) { return move.elapsed >= move_time; });

    for (auto &glide : _glides)
    {
      UiElement *element = ElementOf(UiHandle{glide.element});
      if (element == nullptr)
      {
        glide.speed_x = 0.0f;
        glide.speed_y = 0.0f;
        continue;
      }

      const bool moved = element->SetScroll(
        element->GetScrollX() + glide.speed_x * seconds,
        element->GetScrollY() + glide.speed_y * seconds);

      if (moved) { Emit(*element, "scroll"); }

      const float slowed = std::exp(-friction * seconds);
      glide.speed_x *= slowed;
      glide.speed_y *= slowed;

      // at the end of what there is to scroll, it stops
      if (!moved)
      {
        glide.speed_x = 0.0f;
        glide.speed_y = 0.0f;
      }
    }

    std::erase_if(_glides, [](const Glide &glide)
    {
      return std::abs(glide.speed_x) < least_speed && std::abs(glide.speed_y) < least_speed;
    });
  }
} // neon
