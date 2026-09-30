#ifndef FIXED_CLOCK_HPP
#define FIXED_CLOCK_HPP

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace neon
{
  /// Turns the time of frames, which is whatever it is, into steps that all
  /// have the same length.
  ///
  /// What happens in a step does not depend on how many frames are drawn in
  /// a second. A game that runs at 30 frames per second takes two steps in a
  /// frame. One that runs at 240 takes a step in every fourth frame. Both
  /// have taken the same steps after a second, and are in the same state.
  ///
  /// The time of a frame is collected. Whenever enough for a step has come
  /// together, a step is taken. What is left waits for the next frame.
  class FixedClock
  {
    double _step = 1.0 / 60.0;
    std::size_t _most_steps = 8;
    double _waiting = 0.0;
    std::uint64_t _step_count = 0;
    std::uint64_t _held_back_count = 0;

  public:
    /// How many steps a second has. 60 unless it is set. What is not above
    /// 0 is ignored.
    void SetStepsPerSecond(const double steps_per_second)
    {
      if (!(steps_per_second > 0.0) || !std::isfinite(steps_per_second)) { return; }
      _step = 1.0 / steps_per_second;
    }

    /// The most steps one frame takes. 8 unless it is set. A frame that took
    /// longer than that many steps takes no more than these, and the time
    /// above them is given up. So the world runs slower than the clock on
    /// the wall for a moment. Without it, a frame that took long would ask
    /// for many steps, which take long, and the next frame would ask for
    /// more. 0 is ignored.
    void SetMostStepsPerFrame(const std::size_t most_steps)
    {
      if (most_steps == 0) { return; }
      _most_steps = most_steps;
    }

    /// Seconds a step advances by.
    [[nodiscard]] double GetStep() const { return _step; }

    [[nodiscard]] std::size_t GetMostStepsPerFrame() const { return _most_steps; }

    /// Takes in the time of a frame, in seconds. Returns how many steps are
    /// to be taken in this frame, which can be none.
    std::size_t Advance(const double delta_time)
    {
      if (!(delta_time > 0.0) || !std::isfinite(delta_time)) { return 0; }

      _waiting += delta_time;

      // Sixty frames of a sixtieth of a second do not add up to a second
      // exactly. Without this, the last step of a second would now and then
      // be taken a frame late. A millionth of a step is far more than what
      // adding up loses, and far less than anyone can notice.
      const double slack = _step * 1e-6;
      const double whole = std::floor((_waiting + slack) / _step);

      if (whole > static_cast<double>(_most_steps))
      {
        // the steps above the most are given up. What is left of a step
        // stays, so that what is drawn goes on from where it was
        _held_back_count += static_cast<std::uint64_t>(whole) - _most_steps;
        _waiting = std::fmod(_waiting, _step);
        _step_count += _most_steps;
        return _most_steps;
      }

      _waiting -= whole * _step;
      if (_waiting < 0.0) { _waiting = 0.0; }

      const auto steps = static_cast<std::size_t>(whole);
      _step_count += steps;
      return steps;
    }

    /// How far the time that waits reaches into the next step, from 0 to 1.
    /// What is drawn lies that far between the last two steps.
    [[nodiscard]] double GetBlend() const
    {
      const double blend = _waiting / _step;
      return blend < 0.0 ? 0.0 : blend > 1.0 ? 1.0 : blend;
    }

    /// Steps that were taken since the clock was made.
    [[nodiscard]] std::uint64_t GetStepCount() const { return _step_count; }

    /// Steps that were given up because a frame asked for more than the
    /// most.
    [[nodiscard]] std::uint64_t GetHeldBackCount() const { return _held_back_count; }
  };
} // neon

#endif //FIXED_CLOCK_HPP
