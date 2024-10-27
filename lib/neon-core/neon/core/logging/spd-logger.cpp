#include "spd-logger.hpp"

namespace core
{
  Spd_Logger::Spd_Logger(const std::shared_ptr<spdlog::logger> &logger)
  {
    _logger = logger;
  }

  void Spd_Logger::TraceImpl(const std::string &format, const std::format_args &args)
  {
    const auto result = std::vformat(format, args);
    _logger->trace(result);
  }

  void Spd_Logger::DebugImpl(const std::string &format, const std::format_args &args)
  {
    const auto result = std::vformat(format, args);
    _logger->debug(result);
  }

  void Spd_Logger::InfoImpl(const std::string &format, const std::format_args& args)
  {
    const auto result = std::vformat(format, args);
    _logger->info(result);
  }

  void Spd_Logger::WarnImpl(const std::string &format, const std::format_args &args)
  {
    const auto result = std::vformat(format, args);
    _logger->warn(result);
  }

  void Spd_Logger::ErrorImpl(const std::string &format, const std::format_args &args)
  {
    const auto result = std::vformat(format, args);
    _logger->error(result);
  }

  void Spd_Logger::CriticalImpl(const std::string &format, const std::format_args &args)
  {
    const auto result = std::vformat(format, args);
    _logger->critical(result);
  }
} // core
