#ifndef INPUT_CONTEXT_HPP
#define INPUT_CONTEXT_HPP

#include <../../../../external/glm/glm/glm.hpp>

namespace core
{
  class InputContext
  {
  protected:

    ~InputContext() = default;

  public:
    virtual glm::vec3 GetMovementDirection() = 0;
  };
} // core

#endif //INPUT_CONTEXT_HPP
