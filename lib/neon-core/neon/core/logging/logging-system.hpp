#ifndef LOGGING_SYSTEM_HPP
#define LOGGING_SYSTEM_HPP
#include <memory>

#include "logger.hpp"
#include "logging-context.hpp"
#include "neon/core/application/settings-config.hpp"
#include "spdlog/logger.h"


namespace core
{
  class LoggingSystem final : public LoggingContext
  {
    SettingsConfig _settings_config;
    std::vector<spdlog::sink_ptr> _sinks;
    std::shared_ptr<spdlog::logger> _logger;
  public:
    explicit LoggingSystem(const SettingsConfig& settings_config);

    void Initialize();

    void CleanUp() const;

    std::shared_ptr<Logger> CreateLogger(const std::string &name) override;
  };
} // core

#endif //LOGGING_SYSTEM_HPP
