#ifndef PHYSICS_SYSTEM_HPP
#define PHYSICS_SYSTEM_HPP

#include <algorithm>
#include <memory>
#include <tuple>
#include <vector>

#include <neon/logging/logger.hpp>
#include <neon/runtime/settings-config.hpp>

#include "physics-context.hpp"

namespace neon
{
  /// Base class for physics backends.
  ///
  /// It owns what is the same for every backend, which is keeping the events
  /// for those who read them. A backend simulates, and hands what a step
  /// reported to Report().
  class PhysicsSystem : public PhysicsContext
  {
    std::vector<PhysicsEvent> _step_events;
    std::vector<PhysicsEvent> _frame_events;

  protected:
    SettingsConfig _settings_config;
    std::shared_ptr<Logger> _logger;

    ~PhysicsSystem() = default;

    /// Takes what a step reported. A backend calls this once at the end of
    /// every step, also when nothing was reported.
    ///
    /// The events are put in order, so that the same run hands them over in
    /// the same order whatever order the threads of a backend found them in.
    void Report(std::vector<PhysicsEvent> events)
    {
      std::ranges::stable_sort(events, [](const PhysicsEvent &left, const PhysicsEvent &right)
      {
        // what ended comes first, so that a body that left one trigger and
        // entered another in the same step is never in both
        return std::tuple(left.kind != PhysicsEventKind::Ended, left.first_body, left.second_body)
               < std::tuple(right.kind != PhysicsEventKind::Ended, right.first_body, right.second_body);
      });

      _frame_events.insert(_frame_events.end(), events.begin(), events.end());
      _step_events = std::move(events);
    }

    /// Forgets every event. For a backend that is cleaned up.
    void ForgetEvents()
    {
      _step_events.clear();
      _frame_events.clear();
    }

  public:
    explicit PhysicsSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
    {
      _settings_config = settings_config;
      _logger = logger;
    }

    virtual void Initialize() = 0;

    /// Destroys every body that is left. Safe to call more than once.
    virtual void CleanUp() = 0;

    [[nodiscard]] const std::vector<PhysicsEvent> &GetStepEvents() const final
    {
      return _step_events;
    }

    [[nodiscard]] const std::vector<PhysicsEvent> &GetFrameEvents() const final
    {
      return _frame_events;
    }

    void EndFrame() final
    {
      _frame_events.clear();
    }
  };
} // neon

#endif //PHYSICS_SYSTEM_HPP
