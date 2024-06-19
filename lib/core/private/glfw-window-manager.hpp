#ifndef GLFW_WINDOW_MANAGER_HPP
#define GLFW_WINDOW_MANAGER_HPP
#include "core/window-manager.hpp"

namespace core {

class Glfw_WindowManager final : public WindowManager{
public:
  void Init() override;
};

} // core

#endif //GLFW_WINDOW_MANAGER_HPP
