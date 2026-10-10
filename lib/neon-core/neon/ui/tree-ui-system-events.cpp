#include "tree-ui-system.hpp"

#include <algorithm>

// What happened to elements, and who is told of it.

namespace neon
{
  // Helpers of tree-ui-system-events.cpp, for this file alone.
  namespace
  {
    /// Events that stay at the element they happened to, as in the DOM.
    bool StaysAtItsElement(const std::string &name)
    {
      return name == "pointer_enter" || name == "pointer_leave" || name == "focused" || name == "blurred" ||
             name == "scroll" || name == "animation_ended" || name == "transition_ended";
    }
  }

  UiElement *Tree_UiSystem::ElementOf(const UiHandle handle) const
  {
    const auto found = _elements.find(handle.id);
    return found != _elements.end() ? found->second : nullptr;
  }

  UiHandle Tree_UiSystem::HandleOf(const UiElement *element)
  {
    return element != nullptr ? UiHandle{element->GetId()} : UiHandle{};
  }

  const UiElement *Tree_UiSystem::GetElement(const UiHandle handle) const
  {
    return ElementOf(handle);
  }

  bool Tree_UiSystem::IsAlive(const UiHandle element) const
  {
    return ElementOf(element) != nullptr;
  }

  void Tree_UiSystem::Emit(UiElement &target, const std::string &name, UiElementEvent event)
  {
    const UiDocument *document = DocumentOf(&target);

    event.name = name;
    event.target = HandleOf(&target);
    event.current = event.target;
    event.target_name = target.GetName();
    event.document = document != nullptr ? document->name : "";
    event.bubbles = !StaysAtItsElement(name);
    event.is_stopped = false;

    _element_events.push_back(event);
  }

  void Tree_UiSystem::CollectNotices()
  {
    std::vector<UiElement *> elements;
    elements.swap(_noticing);

    for (UiElement *element : elements)
    {
      element->ClearDirty(UiDirty::Notice);

      // what the player changed last in the frame, for `on_change`
      const UiNotice *changed = nullptr;
      const std::vector<UiNotice> notices = element->TakeNotices();

      for (const auto &notice : notices)
      {
        // what is typed is written back to the value of the game it shows
        if (!notice.binding.empty()) { _values.Set(notice.binding, notice.bound_value); }

        UiElementEvent event;
        event.value = notice.value;
        Emit(*element, notice.name, event);

        if (notice.name == "changed" && notice.by_player) { changed = &notice; }
      }

      // the function is called once a frame, with what the element holds
      // at the end of it, and with the values it names as they are after
      // it wrote its own
      const UiCall *call = element->CallsWhenChanged();
      if (changed == nullptr || call == nullptr) { continue; }

      UiEvent event = EventOf(*element, UiEvent::Kind::Change, call);
      event.value = changed->bound_value;
      _events.push_back(event);
    }
  }

  void Tree_UiSystem::DispatchEvents()
  {
    if (_subscriptions.empty() || _element_events.empty()) { return; }

    // From copies. A listener is free to change anything: to remove
    // elements, to load and unload files, and to stop listening.
    const std::vector<UiElementEvent> events = _element_events;

    for (const auto &happened : events)
    {
      UiElementEvent event = happened;

      const auto tell = [this, &event](const std::uint64_t element)
      {
        const std::vector<Subscription> subscriptions = _subscriptions;

        for (const auto &subscription : subscriptions)
        {
          if (subscription.element != element || subscription.event != event.name) { continue; }

          // one that was taken away by a listener before it is not called
          const bool is_there = std::ranges::any_of(_subscriptions, [&subscription](const Subscription &each)
          {
            return each.id == subscription.id;
          });

          if (is_there && subscription.listener) { subscription.listener(event); }
        }
      };

      // what listens to every element hears of it where it happened
      tell(0);

      // The way up is what it is when the event is told. An element that
      // a listener removed is no longer on it.
      std::uint64_t at = event.target.id;

      while (at != 0 && !event.is_stopped)
      {
        const UiElement *element = ElementOf(UiHandle{at});
        if (element == nullptr) { break; }

        event.current = UiHandle{at};
        tell(at);

        if (!event.bubbles) { break; }

        // asked again, since the listener may have removed the element
        element = ElementOf(UiHandle{at});
        if (element == nullptr) { break; }

        at = element->GetParent() != nullptr ? element->GetParent()->GetId() : 0;
      }
    }
  }

  int Tree_UiSystem::On(const UiHandle element, const std::string &event, const UiListener &listener)
  {
    if (ElementOf(element) == nullptr || !listener || event.empty()) { return 0; }

    _subscriptions.push_back({_next_subscription, element.id, event, listener});
    return _next_subscription++;
  }

  int Tree_UiSystem::OnAny(const std::string &event, const UiListener &listener)
  {
    if (!listener || event.empty()) { return 0; }

    _subscriptions.push_back({_next_subscription, 0, event, listener});
    return _next_subscription++;
  }

  void Tree_UiSystem::Off(const int subscription)
  {
    std::erase_if(_subscriptions, [subscription](const Subscription &each) { return each.id == subscription; });
  }

  const std::vector<UiElementEvent> &Tree_UiSystem::GetElementEvents() const
  {
    return _element_events;
  }
} // neon
