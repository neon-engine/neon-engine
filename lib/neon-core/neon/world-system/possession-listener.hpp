#ifndef POSSESSION_LISTENER_HPP
#define POSSESSION_LISTENER_HPP

#include "possession-event.hpp"

namespace neon
{
  /// Whoever is told when an entity is possessed or released: what runs
  /// the scripts and the extensions, which hand it on to their hooks. It is
  /// told at once, inside the call that possesses, so one that calls into
  /// code that may possess in turn keeps the event and hands it on in its
  /// own frame, as ScriptRunning does.
  class PossessionListener
  {
  protected:
    ~PossessionListener() = default;

  public:
    virtual void OnPossession(const PossessionEvent &event) = 0;
  };
} // neon

#endif //POSSESSION_LISTENER_HPP
