#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP


namespace core
{
  class InputManager
  {
  public:
    virtual void CleanUp();
    ~InputManager() = default;
  };
}


#endif //INPUT_MANAGER_HPP
