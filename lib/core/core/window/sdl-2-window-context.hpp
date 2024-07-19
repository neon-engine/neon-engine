#ifndef SDL_2_WINDOW_CONTEXT_HPP
#define SDL_2_WINDOW_CONTEXT_HPP


namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class SDL2_WindowContext
  {
  protected:
    ~SDL2_WindowContext() = default;

  public:
    virtual void SignalToClose() = 0;
  };
}


#endif //SDL_2_WINDOW_CONTEXT_HPP
