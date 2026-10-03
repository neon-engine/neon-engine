#ifndef RECORDING_LOGGING_CONTEXT_HPP
#define RECORDING_LOGGING_CONTEXT_HPP

#include <map>
#include <memory>
#include <string>

#include <neon/logging/logging-context.hpp>

#include "recording-logger.hpp"

namespace neon::testing
{
  /// Hands out loggers that keep what they are told, one for each name, so
  /// that a test can ask what was logged under a name.
  class RecordingLoggingContext final : public LoggingContext
  {
    std::map<std::string, std::shared_ptr<RecordingLogger>> _loggers;

  public:
    std::shared_ptr<Logger> CreateLogger(const std::string &name) override
    {
      return Of(name);
    }

    /// The logger of a name, which is made when it is first asked for.
    std::shared_ptr<RecordingLogger> Of(const std::string &name)
    {
      auto &logger = _loggers[name];
      if (logger == nullptr) { logger = std::make_shared<RecordingLogger>(); }
      return logger;
    }
  };
} // neon::testing

#endif //RECORDING_LOGGING_CONTEXT_HPP
