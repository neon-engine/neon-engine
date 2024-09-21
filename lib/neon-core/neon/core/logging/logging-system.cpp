#include "logging-system.hpp"

#include "spd-logger.hpp"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

namespace core
{
  LoggingSystem::LoggingSystem(const SettingsConfig &settings_config)
  {
    _settings_config = settings_config;
  }

  void LoggingSystem::Initialize()
  {
    _sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    _sinks.push_back(
      std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        _settings_config.logpath,
        _settings_config.log_max_size,
        _settings_config.log_max_files));
    _logger = std::make_shared<spdlog::logger>("LoggingSystem", begin(_sinks), end(_sinks));
    _logger->info("LoggingSystem initialized");
  }

  void LoggingSystem::CleanUp() const
  {
    _logger->warn("LoggingSystem cleanup");
  }

  std::shared_ptr<Logger> LoggingSystem::CreateLogger(const std::string &name)
  {
    _logger->debug("Creating logger for ", name);
    const auto logger = std::make_shared<spdlog::logger>(name, begin(_sinks), end(_sinks));
    return std::make_shared<Spd_Logger>(logger);
  }
} // core
