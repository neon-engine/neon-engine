#ifndef SDL_2_EVENT_LISTENER_HPP
#define SDL_2_EVENT_LISTENER_HPP

// an event of SDL, which only the backend knows
union SDL_Event;

namespace neon
{
  /// What is handed every event of SDL as the input system reads them,
  /// before the input system takes it in: a backend library that reads
  /// the events itself, as neon-imgui does for the tool interface. Nothing
  /// of neon-core sees this; it is between the backends.
  // ReSharper disable once CppInconsistentNaming
  class SDL2_EventListener
  {
  protected:
    ~SDL2_EventListener() = default;

  public:
    virtual void HandleEvent(const SDL_Event &event) = 0;
  };
} // neon

#endif //SDL_2_EVENT_LISTENER_HPP
