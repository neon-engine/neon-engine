#ifndef LOGGING_SYSTEM_HPP
#define LOGGING_SYSTEM_HPP
#include <memory>

#include "deferred-file-sink.hpp"
#include "log-file-target.hpp"
#include "logger.hpp"
#include "logging-context.hpp"
#include "neon/runtime/settings-config.hpp"
#include "spdlog/logger.h"


namespace neon
{
  /// Hands out loggers, which write to the console and to one log file.
  ///
  /// It starts before anything else, so that everything can log, and before
  /// the file system, which knows where the log file goes. The console is
  /// written to from the start. What is logged before the file is opened is
  /// held back and written into it once it is, see LogFileTarget.
  class LoggingSystem final : public LoggingContext, public LogFileTarget
  {
    SettingsConfig _settings_config;
    std::shared_ptr<DeferredFileSink> _file_sink;
    std::vector<spdlog::sink_ptr> _sinks;
    std::shared_ptr<spdlog::logger> _logger;
  public:
    explicit LoggingSystem(const SettingsConfig& settings_config);

    void Initialize();

    void CleanUp() const;

    std::shared_ptr<Logger> CreateLogger(const std::string &name) override;

    /// Call after Initialize().
    bool OpenLogFile(const std::string &native_path) override;

    /// Call after Initialize().
    void GoWithoutLogFile() override;
  };
} // neon

#endif //LOGGING_SYSTEM_HPP
