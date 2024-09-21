#include "spd-logger.hpp"

namespace core
{
  Spd_Logger::Spd_Logger(const std::shared_ptr<spdlog::logger> &logger)
  {
    _logger = logger;
  }

  void Spd_Logger::DebugImpl(const std::string &format, std::format_args args)
  {
    const auto result = std::vformat(format, args);
    _logger->debug(result);
  }
} // core
