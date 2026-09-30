#ifndef RECORDING_LOGGER_HPP
#define RECORDING_LOGGER_HPP

#include <algorithm>
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <neon/logging/logger.hpp>

namespace neon::testing
{
  enum class LogLevel
  {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical
  };

  struct LogEntry
  {
    LogLevel level;
    std::string message;
  };

  /// A logger that keeps what it is told, so that a test can ask what was
  /// logged. Nothing is printed and no file is written.
  class RecordingLogger final : public Logger
  {
    std::vector<LogEntry> _entries;

    void Record(const LogLevel level, const std::string &format, const std::format_args &args)
    {
      _entries.push_back({level, std::vformat(format, args)});
    }

  protected:
    void TraceImpl(const std::string &format, const std::format_args &args) override
    {
      Record(LogLevel::Trace, format, args);
    }

    void DebugImpl(const std::string &format, const std::format_args &args) override
    {
      Record(LogLevel::Debug, format, args);
    }

    void InfoImpl(const std::string &format, const std::format_args &args) override
    {
      Record(LogLevel::Info, format, args);
    }

    void WarnImpl(const std::string &format, const std::format_args &args) override
    {
      Record(LogLevel::Warn, format, args);
    }

    void ErrorImpl(const std::string &format, const std::format_args &args) override
    {
      Record(LogLevel::Error, format, args);
    }

    void CriticalImpl(const std::string &format, const std::format_args &args) override
    {
      Record(LogLevel::Critical, format, args);
    }

    std::shared_ptr<Logger> CreateChildLogger(const std::string &name) override
    {
      return std::make_shared<RecordingLogger>();
    }

  public:
    /// Everything that was logged, in the order it was logged in.
    [[nodiscard]] const std::vector<LogEntry> &Entries() const
    {
      return _entries;
    }

    /// Number of messages that were logged at the level.
    [[nodiscard]] std::size_t Count(const LogLevel level) const
    {
      return static_cast<std::size_t>(std::ranges::count_if(_entries, [level](const LogEntry &entry)
      {
        return entry.level == level;
      }));
    }

    /// Whether a message at the level holds the text.
    [[nodiscard]] bool Contains(const LogLevel level, const std::string_view text) const
    {
      return std::ranges::any_of(_entries, [level, text](const LogEntry &entry)
      {
        return entry.level == level && entry.message.find(text) != std::string::npos;
      });
    }

    /// The messages that were logged at the level, one per line. For the
    /// text a failed expectation prints.
    [[nodiscard]] std::string Messages(const LogLevel level) const
    {
      std::string messages;
      for (const auto &[entry_level, message] : _entries)
      {
        if (entry_level == level) { messages += message + "\n"; }
      }
      return messages;
    }

    void Clear()
    {
      _entries.clear();
    }
  };
} // neon::testing

#endif //RECORDING_LOGGER_HPP
