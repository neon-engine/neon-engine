#ifndef SCRIPT_SYSTEM_HPP
#define SCRIPT_SYSTEM_HPP

#include "script-context.hpp"

namespace neon
{
  /// The base class of the backends of the scripts: the context, with the
  /// lifecycle a backend needs. Only `main.cpp` and the runtime hold it.
  class ScriptSystem : public ScriptContext
  {
  protected:
    ~ScriptSystem() = default;

  public:
    /// Starts the language. Call before anything else.
    virtual void Initialize() = 0;

    /// Forgets every script and ends the language. Safe to call more than
    /// once.
    virtual void CleanUp() = 0;
  };
} // neon

#endif //SCRIPT_SYSTEM_HPP
