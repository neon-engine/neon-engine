#ifndef SPD_LOGGER_HPP
#define SPD_LOGGER_HPP
#include "logger.hpp"
#include "spdlog/spdlog.h"

namespace core
{
  class Spd_Logger final : public Logger
  {
    std::shared_ptr<spdlog::logger> _logger;

  public:
    explicit Spd_Logger(const std::shared_ptr<spdlog::logger> &logger);

  protected:
    void TraceImpl(const std::string &format, const std::format_args &args) override;

    void DebugImpl(const std::string &format, const std::format_args &args) override;

    void InfoImpl(const std::string &format, const std::format_args &args) override;

    void WarnImpl(const std::string &format, const std::format_args &args) override;

    void ErrorImpl(const std::string &format, const std::format_args &args) override;

    void CriticalImpl(const std::string &format, const std::format_args &args) override;
  };
} // core

#endif //SPD_LOGGER_HPP
