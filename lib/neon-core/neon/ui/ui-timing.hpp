#ifndef UI_TIMING_HPP
#define UI_TIMING_HPP

#include <string>

// How a change goes from its start to its end over time, as
// https://www.w3.org/TR/css-easing-1/ defines it.

namespace neon
{
  /// A timing function of CSS: what part of the way is covered at a part of
  /// the time.
  struct UiTimingFunction
  {
    enum class Kind
    {
      Linear = 0,
      CubicBezier,
      Steps
    };

    /// Where the jumps of `steps()` are.
    enum class StepPosition
    {
      /// The first jump is at the start.
      JumpStart = 0,
      /// The last jump is at the end. It is what `steps(4)` means.
      JumpEnd,
      /// No jump at the start and none at the end.
      JumpNone,
      /// A jump at the start and one at the end.
      JumpBoth
    };

    Kind kind = Kind::CubicBezier;

    /// The two points of the curve that are not its ends. It starts as
    /// `ease`, which is what CSS starts with.
    float x1 = 0.25f;
    float y1 = 0.1f;
    float x2 = 0.25f;
    float y2 = 1.0f;

    int steps = 1;
    StepPosition step_position = StepPosition::JumpEnd;

    static UiTimingFunction Linear();

    static UiTimingFunction Ease();

    static UiTimingFunction EaseIn();

    static UiTimingFunction EaseOut();

    static UiTimingFunction EaseInOut();

    static UiTimingFunction CubicBezier(float x1, float y1, float x2, float y2);

    static UiTimingFunction Steps(int steps, StepPosition position);

    /// The part of the way at a part of the time, which is from 0 to 1. It
    /// may be below 0 and above 1 for a curve that swings out. `before`
    /// says that the change has not started yet, which decides what a jump
    /// at the very start comes to.
    [[nodiscard]] float At(float time, bool before = false) const;

    bool operator==(const UiTimingFunction &other) const = default;
  };

  /// Reads `linear`, `ease`, `ease-in`, `ease-out`, `ease-in-out`,
  /// `step-start`, `step-end`, `cubic-bezier(x1, y1, x2, y2)`, and
  /// `steps(n, position)`.
  [[nodiscard]] bool ParseCssTimingFunction(const std::string &text, UiTimingFunction &function);

  /// Reads a time such as `0.2s` and `150ms`, in seconds. A time has a
  /// unit, as CSS says, even when it is 0: in `animation`, a 0 without one
  /// is how often the animation runs.
  [[nodiscard]] bool ParseCssTime(const std::string &text, float &seconds);

  /// As CSS writes it.
  [[nodiscard]] std::string FormatCssTimingFunction(const UiTimingFunction &function);
} // neon

#endif //UI_TIMING_HPP
