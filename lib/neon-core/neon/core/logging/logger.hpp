#ifndef LOGGER_HPP
#define LOGGER_HPP
#include <format>
#include <memory>
#include <string>

namespace neon
{
  class Logger
  {
  protected:
    ~Logger() = default;

    virtual void TraceImpl(const std::string& format, const std::format_args &args) = 0;
    virtual void DebugImpl(const std::string& format, const std::format_args &args) = 0;
    virtual void InfoImpl(const std::string& format, const std::format_args &args) = 0;
    virtual void WarnImpl(const std::string& format, const std::format_args &args) = 0;
    virtual void ErrorImpl(const std::string& format, const std::format_args &args) = 0;
    virtual void CriticalImpl(const std::string& format, const std::format_args &args) = 0;
    virtual std::shared_ptr<Logger> CreateChildLogger(const std::string &name) = 0;

  public:
    template<typename... Args>
    void Trace(const std::string& format, Args&&... args)
    {
      auto args_pack = std::make_format_args(std::forward<Args>(args)...);
      TraceImpl(format, args_pack);
    }

    template<typename... Args>
    void Debug(const std::string& format, Args&&... args)
    {
      auto args_pack = std::make_format_args(std::forward<Args>(args)...);
      DebugImpl(format, args_pack);
    }

    template<typename... Args>
    void Info(const std::string& format, Args&&... args)
    {
      auto args_pack = std::make_format_args(std::forward<Args>(args)...);
      InfoImpl(format, args_pack);
    }

    template<typename... Args>
    void Warn(const std::string& format, Args&&... args)
    {
      auto args_pack = std::make_format_args(std::forward<Args>(args)...);
      WarnImpl(format, args_pack);
    }

    template<typename... Args>
    void Error(const std::string& format, Args&&... args)
    {
      auto args_pack = std::make_format_args(std::forward<Args>(args)...);
      ErrorImpl(format, args_pack);
    }

    template<typename... Args>
    void Critical(const std::string& format, Args&&... args)
    {
      auto args_pack = std::make_format_args(std::forward<Args>(args)...);
      CriticalImpl(format, args_pack);
    }
  };
} // neon

#endif //LOGGER_HPP
