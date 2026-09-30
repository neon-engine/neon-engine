#ifndef UI_ANIMATOR_HPP
#define UI_ANIMATOR_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "ui-declarations.hpp"
#include "ui-element.hpp"
#include "ui-properties.hpp"
#include "ui-style-sheets.hpp"

namespace neon
{
  /// Moves the properties of elements over time: the transitions of
  /// https://www.w3.org/TR/css-transitions-1/ and the animations of
  /// https://www.w3.org/TR/css-animations-1/.
  ///
  /// It works on the values of properties and knows nothing of what a
  /// property is. How a value goes from one to another follows from what it
  /// holds, which InterpolateUiValue() decides. A property that is added to
  /// the table of properties is therefore animated without anything being
  /// added here.
  ///
  /// Time is what it is told to advance by and nothing else. The same steps
  /// of time give the same values, on every machine and every time.
  class UiAnimator
  {
  public:
    /// Something that came to its end.
    struct Ended
    {
      std::uint64_t element = 0;

      /// The property of a transition, or the name of the keyframes.
      std::string name;

      bool is_transition = false;
    };

    /// What an animation is given next to its keyframes, as the
    /// properties of CSS say it.
    struct Options
    {
      float duration = 0.0f;
      float delay = 0.0f;

      /// Below 0 stands for `infinite`.
      float iteration_count = 1.0f;

      UiAnimationDirection direction = UiAnimationDirection::Normal;
      UiAnimationFillMode fill_mode = UiAnimationFillMode::None;
      UiAnimationPlayState play_state = UiAnimationPlayState::Running;
      UiTimingFunction timing_function = UiTimingFunction::Ease();

      bool operator==(const Options &other) const = default;
    };

    /// Works out what a declaration of a keyframe comes to for an element.
    using Context = std::function<UiDeclarationContext(const UiElement &element)>;

  private:
    struct Transition
    {
      const UiProperty *property = nullptr;
      UiPropertyValue from;
      UiPropertyValue to;

      float duration = 0.0f;
      float delay = 0.0f;
      UiTimingFunction timing_function;

      // seconds since it was started
      float elapsed = 0.0f;

      // what a transition that is turned around on its way is shortened
      // by, and where it started from before it was turned around
      float reversing_factor = 1.0f;
      UiPropertyValue reversing_from;
    };

    /// A value of a property at a part of the way of an animation.
    struct Stop
    {
      float offset = 0.0f;
      UiPropertyValue value;

      // how the way to the next stop takes its time
      UiTimingFunction timing_function;
    };

    struct Track
    {
      const UiProperty *property = nullptr;
      std::vector<Stop> stops;
    };

    struct Animation
    {
      std::string name;
      Options options;

      // whether a game started it, and not the style of the element
      bool is_started_by_hand = false;

      float elapsed = 0.0f;
      bool has_ended = false;
      bool is_told = false;

      std::vector<Track> tracks;
      bool needs_tracks = true;
    };

    struct State
    {
      std::vector<Transition> transitions;
      std::vector<Animation> animations;

      // what the element is drawn with was changed by this
      bool is_applied = false;
    };

    std::map<std::uint64_t, State> _states;

    [[nodiscard]] static float ProgressOf(const Transition &transition);

    /// Where an animation is on its way, from 0 to 1, or below 0 when it
    /// has no say at this time.
    [[nodiscard]] static float ProgressOf(const Animation &animation, bool &is_before);

    static void MakeTracks(
      Animation &animation,
      const UiElement &element,
      const UiStyleSheets::Keyframes &keyframes,
      const UiDeclarationContext &context);

    [[nodiscard]] static UiPropertyValue ValueOf(const Track &track, float progress, bool is_before);

  public:
    /// Called when the cascade gave an element another style. It starts
    /// the transitions of what changed, and the animations the style
    /// names, and takes away those it names no longer. `had_style` is
    /// whether the element had a style before: what it starts with moves
    /// from nowhere.
    void StyleChanged(UiElement &element, const UiStyle &before, const UiStyle &after, bool had_style);

    /// Writes the style an element is drawn with: what the cascade came
    /// to, with what is on its way in place of it.
    void Apply(UiElement &element, const UiStyleSheets *sheets, const Context &context);

    /// Moves everything on by a time in seconds. `find` hands out the
    /// element of a number, or nullptr for one that is gone.
    void Advance(
      float seconds,
      const std::function<UiElement *(std::uint64_t)> &find,
      const std::function<const UiStyleSheets *(const UiElement &)> &sheets_of,
      const Context &context,
      std::vector<Ended> &ended);

    /// Starts keyframes on an element by hand, next to what its style
    /// runs. One of the same name that was started by hand before starts
    /// again.
    void StartByHand(UiElement &element, const std::string &name, const Options &options);

    /// Stops what was started by hand: the keyframes of a name, or all of
    /// them with an empty name. Returns whether there was something to
    /// stop.
    bool StopByHand(UiElement &element, const std::string &name);

    /// Forgets an element that is gone.
    void Forget(std::uint64_t element);

    void Clear();

    /// Whether something of an element is on its way.
    [[nodiscard]] bool IsMoving(std::uint64_t element) const;

    /// How many elements have something on its way.
    [[nodiscard]] std::size_t GetMovingCount() const;
  };
} // neon

#endif //UI_ANIMATOR_HPP
