#ifndef APPLICATION_HPP
#define APPLICATION_HPP

namespace core
{
  class Application
  {
  public:
    virtual ~Application() = default;

    virtual void Run();
  };
} // core

#endif //APPLICATION_HPP
