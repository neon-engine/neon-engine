#ifndef WINDOW_MANAGER_HPP
#define WINDOW_MANAGER_HPP

namespace core
{
  class WindowManager
  {
  protected:
    WindowManager() = default;
    ~WindowManager() = default;
  public:
    virtual void Init() = 0;
  };
} // core

#endif //WINDOW_MANAGER_HPP
