#include "ui-animator.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  // Helpers of UiAnimator, for this file alone.
  namespace
  {
    /// The value of a list of CSS at a place, which goes through the list
    /// again from its start when the list is shorter.
    template<typename T>
    T At(const std::vector<T> &list, const std::size_t index, const T &otherwise)
    {
      return list.empty() ? otherwise : list[index % list.size()];
    }

    /// The properties a name of `transition_property` stands for.
    std::vector<const UiProperty *> PropertiesOf(const std::string &name)
    {
      const UiProperties &properties = UiProperties::Get();
      std::vector<const UiProperty *> found;

      if (name == "all")
      {
        for (const auto &property : properties.GetAll())
        {
          if (!property.IsShorthand()) { found.push_back(&property); }
        }
        return found;
      }

      if (const UiProperty *property = properties.Find(name); property != nullptr)
      {
        return properties.GetLonghands(*property);
      }

      return found;
    }

    bool IsAnimationProperty(const std::string &name)
    {
      return name.starts_with("animation") || name.starts_with("transition");
    }
  }

  float UiAnimator::ProgressOf(const Transition &transition)
  {
    const float local = transition.elapsed - transition.delay;

    if (local <= 0.0f) { return 0.0f; }
    if (transition.duration <= 0.0f || local >= transition.duration) { return 1.0f; }

    return transition.timing_function.At(local / transition.duration);
  }

  float UiAnimator::ProgressOf(const Animation &animation, bool &is_before)
  {
    const Options &options = animation.options;
    const float local = animation.elapsed - options.delay;

    const bool fills_backwards = options.fill_mode == UiAnimationFillMode::Backwards ||
                                 options.fill_mode == UiAnimationFillMode::Both;
    const bool fills_forwards = options.fill_mode == UiAnimationFillMode::Forwards ||
                                options.fill_mode == UiAnimationFillMode::Both;

    const bool is_endless = options.iteration_count < 0.0f;
    const float active = is_endless ? 0.0f : options.duration * options.iteration_count;

    is_before = local < 0.0f;

    // how many runs are behind it, and how far the one it is in has come
    float runs = 0.0f;

    if (is_before)
    {
      if (!fills_backwards) { return -1.0f; }
      runs = 0.0f;
    } else if (!is_endless && local >= active)
    {
      if (!fills_forwards) { return -1.0f; }
      runs = options.iteration_count;
    } else
    {
      runs = options.duration > 0.0f ? local / options.duration : 0.0f;
    }

    float run = std::floor(runs);
    float progress = runs - run;

    // where a run ends is the end of that run, and not the start of the
    // next, when nothing follows
    const bool is_at_end = !is_before && !is_endless && local >= active;
    if (is_at_end && progress == 0.0f && runs > 0.0f)
    {
      run -= 1.0f;
      progress = 1.0f;
    }

    const bool is_odd = static_cast<long>(run) % 2 != 0;

    bool runs_backwards = false;
    switch (options.direction)
    {
      case UiAnimationDirection::Reverse:
        runs_backwards = true;
        break;
      case UiAnimationDirection::Alternate:
        runs_backwards = is_odd;
        break;
      case UiAnimationDirection::AlternateReverse:
        runs_backwards = !is_odd;
        break;
      default:
        break;
    }

    return runs_backwards ? 1.0f - progress : progress;
  }

  UiPropertyValue UiAnimator::ValueOf(const Track &track, const float progress, const bool is_before)
  {
    const auto &stops = track.stops;

    if (stops.size() == 1) { return stops[0].value; }

    // the two stops the place is between
    std::size_t next = 1;
    while (next + 1 < stops.size() && stops[next].offset <= progress) { next++; }

    // of two at the same place, the later is where a way starts
    while (next + 1 < stops.size() && stops[next].offset == stops[next + 1].offset && stops[next].offset <= progress)
    {
      next++;
    }

    const Stop &from = stops[next - 1];
    const Stop &to = stops[next];

    const float length = to.offset - from.offset;
    if (length <= 0.0f) { return progress >= to.offset ? to.value : from.value; }

    const float local = std::clamp((progress - from.offset) / length, 0.0f, 1.0f);

    // what cannot be moved switches halfway, as CSS says
    return InterpolateUiValue(from.value, to.value, from.timing_function.At(local, is_before));
  }

  void UiAnimator::MakeTracks(
    Animation &animation,
    const UiElement &element,
    const UiStyleSheets::Keyframes &keyframes,
    const UiDeclarationContext &context)
  {
    animation.tracks.clear();
    animation.needs_tracks = false;

    const UiStyle &computed = element.GetComputedStyle();
    const UiProperties &properties = UiProperties::Get();

    for (const auto &frame : keyframes.frames)
    {
      // what the frame writes, read into the style of the element, from
      // which the values it comes to are taken
      UiStyle style = computed;
      std::vector<const UiProperty *> written;
      UiTimingFunction timing_function = animation.options.timing_function;

      for (const auto &declaration : frame.declarations)
      {
        if (declaration.IsCustomProperty()) { continue; }

        if (declaration.name == "animation-timing-function")
        {
          UiTimingFunction read;
          if (ParseCssTimingFunction(declaration.value, read)) { timing_function = read; }
          continue;
        }

        if (IsAnimationProperty(declaration.name)) { continue; }

        std::vector<std::string> unheard;
        if (!ApplyUiDeclaration(declaration.name, declaration.value, context, "", style, unheard)) { continue; }

        const UiProperty *property = properties.Find(declaration.name);
        if (property == nullptr) { continue; }

        for (const UiProperty *each : properties.GetLonghands(*property))
        {
          if (std::ranges::find(written, each) == written.end()) { written.push_back(each); }
        }
      }

      for (const UiProperty *property : written)
      {
        auto track = std::ranges::find_if(animation.tracks, [property](const Track &each)
        {
          return each.property == property;
        });

        if (track == animation.tracks.end())
        {
          animation.tracks.push_back({property, {}});
          track = animation.tracks.end() - 1;
        }

        // of two frames at one place, the later one holds
        if (!track->stops.empty() && track->stops.back().offset == frame.offset)
        {
          track->stops.back() = {frame.offset, property->get(style), timing_function};
        } else
        {
          track->stops.push_back({frame.offset, property->get(style), timing_function});
        }
      }
    }

    // Where the keyframes say nothing about the start or the end, the
    // value the element has is what it starts from and ends at.
    for (auto &track : animation.tracks)
    {
      const UiPropertyValue own = track.property->get(computed);

      if (track.stops.front().offset > 0.0f)
      {
        track.stops.insert(track.stops.begin(), {0.0f, own, animation.options.timing_function});
      }

      if (track.stops.back().offset < 1.0f)
      {
        track.stops.push_back({1.0f, own, animation.options.timing_function});
      }
    }
  }

  void UiAnimator::StyleChanged(
    UiElement &element,
    const UiStyle &before,
    const UiStyle &after,
    const bool had_style)
  {
    const std::uint64_t id = element.GetId();
    if (id == 0) { return; }

    const auto known = _states.find(id);
    const bool names_animations = !after.animations.names.empty();

    // most elements move nothing, and are not kept
    if (known == _states.end() && !names_animations && (!had_style || !after.transitions.IsUsed())) { return; }

    State &state = _states[id];

    // Transitions: for every property whose value changed and whose
    // change takes time.
    if (had_style)
    {
      const UiTransitions &transitions = after.transitions;

      // the transition that is named last for a property is the one that
      // holds
      std::vector<std::pair<const UiProperty *, std::size_t>> named;

      for (std::size_t i = 0; i < transitions.properties.size(); i++)
      {
        if (transitions.properties[i] == "none") { continue; }

        for (const UiProperty *property : PropertiesOf(transitions.properties[i]))
        {
          std::erase_if(named, [property](const auto &each) { return each.first == property; });
          named.emplace_back(property, i);
        }
      }

      // what is on its way for a property that takes no time any more
      // ends where it is
      std::erase_if(state.transitions, [&](const Transition &transition)
      {
        return std::ranges::none_of(named, [&transition](const auto &each)
        {
          return each.first == transition.property;
        });
      });

      for (const auto &[property, index] : named)
      {
        const UiPropertyValue was = property->get(before);
        const UiPropertyValue now = property->get(after);

        const auto running = std::ranges::find_if(state.transitions, [property](const Transition &transition)
        {
          return transition.property == property;
        });

        if (running != state.transitions.end())
        {
          // on its way to where it is to go already
          if (running->to == now) { continue; }
        } else if (was == now)
        {
          continue;
        }

        const float duration = At(transitions.durations, index, 0.0f);
        const float delay = At(transitions.delays, index, 0.0f);

        // Where it starts from: where it is, which for one that is on its
        // way is between its ends.
        UiPropertyValue from = was;
        float reversing_factor = 1.0f;
        UiPropertyValue reversing_from = was;
        float shortened_delay = delay;

        if (running != state.transitions.end())
        {
          const float progress = ProgressOf(*running);
          from = InterpolateUiValue(running->from, running->to, progress);
          reversing_from = from;

          // Turned around on its way, it takes as long as it took to get
          // where it is, and not as long as all of the way.
          if (running->reversing_from == now)
          {
            const float local = running->elapsed - running->delay;
            const float done = running->duration > 0.0f
              ? running->timing_function.At(std::clamp(local / running->duration, 0.0f, 1.0f))
              : 1.0f;

            reversing_factor = std::clamp(
              std::abs(done * running->reversing_factor + (1.0f - running->reversing_factor)), 0.0f, 1.0f);

            reversing_from = running->to;

            if (delay < 0.0f) { shortened_delay = delay * reversing_factor; }
          }

          state.transitions.erase(running);
        }

        // what cannot be moved changes at once, as CSS says of
        // transitions
        if (duration + std::max(0.0f, delay) <= 0.0f || !CanInterpolateUiValue(from, now) || from == now)
        {
          continue;
        }

        Transition transition;
        transition.property = property;
        transition.from = from;
        transition.to = now;
        transition.duration = duration * reversing_factor;
        transition.delay = shortened_delay;
        transition.timing_function = At(transitions.timing_functions, index, UiTimingFunction::Ease());
        transition.reversing_factor = reversing_factor;
        transition.reversing_from = reversing_from;

        state.transitions.push_back(transition);
      }
    }

    // Animations: those the style names, in the order it names them.
    const UiAnimations &animations = after.animations;

    std::vector<Animation> kept;

    for (std::size_t i = 0; i < animations.names.size(); i++)
    {
      const std::string &name = animations.names[i];
      if (name == "none") { continue; }

      Options options;
      options.duration = At(animations.durations, i, 0.0f);
      options.delay = At(animations.delays, i, 0.0f);
      options.iteration_count = At(animations.iteration_counts, i, 1.0f);
      options.direction = At(animations.directions, i, UiAnimationDirection::Normal);
      options.fill_mode = At(animations.fill_modes, i, UiAnimationFillMode::None);
      options.play_state = At(animations.play_states, i, UiAnimationPlayState::Running);
      options.timing_function = At(animations.timing_functions, i, UiTimingFunction::Ease());

      // one that runs goes on, with what is said about it now
      const auto running = std::ranges::find_if(state.animations, [&name](const Animation &animation)
      {
        return !animation.is_started_by_hand && animation.name == name;
      });

      Animation animation;

      if (running != state.animations.end())
      {
        animation = *running;
        state.animations.erase(running);
      }

      animation.name = name;
      animation.options = options;
      animation.needs_tracks = true;

      kept.push_back(animation);
    }

    // what a game started stays, behind what the style names
    for (const auto &animation : state.animations)
    {
      if (!animation.is_started_by_hand) { continue; }

      kept.push_back(animation);
      kept.back().needs_tracks = true;
    }

    state.animations = kept;

    if (state.transitions.empty() && state.animations.empty() && !state.is_applied) { _states.erase(id); }
  }

  void UiAnimator::Apply(UiElement &element, const UiStyleSheets *sheets, const Context &context)
  {
    const auto found = _states.find(element.GetId());
    if (found == _states.end()) { return; }

    State &state = found->second;

    UiStyle style = element.GetComputedStyle();
    bool is_applied = false;

    for (auto &animation : state.animations)
    {
      if (animation.needs_tracks)
      {
        const UiStyleSheets::Keyframes *keyframes = sheets != nullptr ? sheets->FindKeyframes(animation.name) : nullptr;

        if (keyframes != nullptr)
        {
          MakeTracks(animation, element, *keyframes, context(element));
        } else
        {
          animation.tracks.clear();
          animation.needs_tracks = false;
        }
      }

      bool is_before = false;
      const float progress = ProgressOf(animation, is_before);
      if (progress < 0.0f) { continue; }

      for (const auto &track : animation.tracks)
      {
        track.property->set(style, ValueOf(track, progress, is_before));
        is_applied = true;
      }
    }

    for (const auto &transition : state.transitions)
    {
      transition.property->set(
        style, InterpolateUiValue(transition.from, transition.to, ProgressOf(transition)));
      is_applied = true;
    }

    if (is_applied || state.is_applied) { element.SetUsedStyle(style); }
    state.is_applied = is_applied;
  }

  void UiAnimator::Advance(
    const float seconds,
    const std::function<UiElement *(std::uint64_t)> &find,
    const std::function<const UiStyleSheets *(const UiElement &)> &sheets_of,
    const Context &context,
    std::vector<Ended> &ended)
  {
    std::vector<std::uint64_t> gone;

    for (auto &[id, state] : _states)
    {
      UiElement *element = find(id);
      if (element == nullptr)
      {
        gone.push_back(id);
        continue;
      }

      bool moves = false;

      for (auto &transition : state.transitions)
      {
        transition.elapsed += seconds;
        moves = true;
      }

      for (auto &animation : state.animations)
      {
        if (animation.options.play_state == UiAnimationPlayState::Paused || animation.has_ended) { continue; }

        animation.elapsed += seconds;
        moves = true;

        const Options &options = animation.options;
        const bool is_endless = options.iteration_count < 0.0f;

        if (!is_endless && animation.elapsed - options.delay >= options.duration * options.iteration_count)
        {
          animation.has_ended = true;
        }
      }

      if (moves)
      {
        // the style is asked for first, so that what it starts is started
        (void) element->GetStyle();

        Apply(*element, sheets_of(*element), context);

        // what came to its end says so, behind the frame that shows its
        // end
        std::erase_if(state.transitions, [&](const Transition &transition)
        {
          if (transition.elapsed - transition.delay < transition.duration) { return false; }

          ended.push_back({id, transition.property->name, true});
          return true;
        });

        for (auto &animation : state.animations)
        {
          if (!animation.has_ended || animation.is_told) { continue; }

          animation.is_told = true;
          ended.push_back({id, animation.name, false});
        }

        // what a game started is gone when it ended, unless it holds what
        // it ended at
        std::erase_if(state.animations, [](const Animation &animation)
        {
          const bool holds = animation.options.fill_mode == UiAnimationFillMode::Forwards ||
                             animation.options.fill_mode == UiAnimationFillMode::Both;

          return animation.is_started_by_hand && animation.has_ended && !holds;
        });
      }

      if (state.transitions.empty() && state.animations.empty())
      {
        // what it was drawn with is what the cascade came to again
        if (state.is_applied) { Apply(*element, sheets_of(*element), context); }
        gone.push_back(id);
      }
    }

    for (const std::uint64_t id : gone) { _states.erase(id); }
  }

  void UiAnimator::StartByHand(UiElement &element, const std::string &name, const Options &options)
  {
    State &state = _states[element.GetId()];

    std::erase_if(state.animations, [&name](const Animation &animation)
    {
      return animation.is_started_by_hand && animation.name == name;
    });

    Animation animation;
    animation.name = name;
    animation.options = options;
    animation.is_started_by_hand = true;

    state.animations.push_back(animation);
  }

  bool UiAnimator::StopByHand(UiElement &element, const std::string &name)
  {
    const auto found = _states.find(element.GetId());
    if (found == _states.end()) { return false; }

    const std::size_t stopped = std::erase_if(found->second.animations, [&name](const Animation &animation)
    {
      return animation.is_started_by_hand && (name.empty() || animation.name == name);
    });

    return stopped > 0;
  }

  void UiAnimator::Forget(const std::uint64_t element)
  {
    _states.erase(element);
  }

  void UiAnimator::Clear()
  {
    _states.clear();
  }

  bool UiAnimator::IsMoving(const std::uint64_t element) const
  {
    const auto found = _states.find(element);
    if (found == _states.end()) { return false; }

    return !found->second.transitions.empty() ||
           std::ranges::any_of(found->second.animations, [](const Animation &animation)
           {
             return !animation.has_ended;
           });
  }

  std::size_t UiAnimator::GetMovingCount() const
  {
    std::size_t count = 0;
    for (const auto &[id, state] : _states)
    {
      if (IsMoving(id)) { count++; }
    }
    return count;
  }
} // neon
