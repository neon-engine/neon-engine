#ifndef INPUT_CONTEXT_HPP
#define INPUT_CONTEXT_HPP

#include "input-state.hpp"

namespace neon
{
  class InputContext
  {
  protected:

    ~InputContext() = default;

  public:
    virtual const InputState& GetInputState() = 0;

    virtual void CenterAndHideCursor() = 0;

    virtual void ShowCursor() = 0;
  };
} // neon

#endif //INPUT_CONTEXT_HPP
