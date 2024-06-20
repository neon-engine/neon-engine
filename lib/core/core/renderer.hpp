#ifndef RENDERER_HPP
#define RENDERER_HPP

namespace core
{
  class Renderer
  {
  protected:
    Window_Info _window_info;
    explicit Renderer(const Window_Info &window_info)
    {
      _window_info = window_info;
    }
    ~Renderer() = default;

  public:
    virtual void Initialize() = 0;
  };
} // core

#endif //RENDERER_HPP
