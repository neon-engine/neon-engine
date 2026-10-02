#include "logging-system.hpp"

#include <exception>

#include "spd-logger.hpp"
#include "spdlog/sinks/stdout_color_sinks-inl.h"

namespace neon
{
  LoggingSystem::LoggingSystem(const SettingsConfig &settings_config)
  {
    _settings_config = settings_config;
  }

  void LoggingSystem::Initialize()
  {
    // Every logger shares the same sinks, so a file that is opened later
    // reaches the loggers that were created before it.
    _file_sink = std::make_shared<DeferredFileSink>(_settings_config.log_max_size, _settings_config.log_max_files);
    _error_count = std::make_shared<ErrorCountSink>();
    _sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    _sinks.push_back(_file_sink);
    _sinks.push_back(_error_count);
    _logger = std::make_shared<spdlog::logger>("LoggingSystem", begin(_sinks), end(_sinks));
    _logger->set_level(spdlog::level::debug);
    _logger->info("LoggingSystem initialized");
  }

  void LoggingSystem::CleanUp() const
  {
    _logger->warn("LoggingSystem cleanup");
  }

  std::shared_ptr<Logger> LoggingSystem::CreateLogger(const std::string &name)
  {
    _logger->debug("Creating logger for {}", name);
    const auto logger = std::make_shared<spdlog::logger>(name, begin(_sinks), end(_sinks));
    logger->set_level(spdlog::level::debug);
    return std::make_shared<Spd_Logger>(logger, this);
  }

  bool LoggingSystem::OpenLogFile(const std::string &native_path)
  {
    try
    {
      _file_sink->Open(native_path);
    } catch (const std::exception &exception)
    {
      // the sink has stopped holding messages, so this reaches the console only
      _logger->error("The log file cannot be opened, logging goes on without one: {}", exception.what());
      return false;
    }

    _logger->info("Logging to {}", native_path);
    return true;
  }

  void LoggingSystem::GoWithoutLogFile()
  {
    _file_sink->GoWithout();
    _logger->warn("Logging goes on without a log file");
  }

  std::size_t LoggingSystem::CountErrors() const
  {
    return _error_count == nullptr ? 0 : _error_count->Count();
  }
} // neon
