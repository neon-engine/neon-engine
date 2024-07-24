#ifndef INPUT_CONTEXT_HPP
#define INPUT_CONTEXT_HPP
#include "input-state.hpp"

namespace core
{
  class InputContext
  {
  protected:

    ~InputContext() = default;

  public:
    virtual const InputState& GetInputState() = 0;
  };
} // core

#endif //INPUT_CONTEXT_HPP
