#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <format>
#include <string>

namespace core
{
  class Logger
  {
  protected:
    ~Logger() = default;

    virtual void DebugImpl(const std::string& format, std::format_args args) = 0;

  public:
    template<typename... Args>
    void Debug(const std::string& format, Args&&... args)
    {
      auto args_pack = std::make_format_args(std::forward<Args>(args)...);
      DebugImpl(format, args_pack);
    }
  };
} // core

#endif //LOGGER_HPP
