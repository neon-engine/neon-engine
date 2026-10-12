#ifndef LOG_HISTORY_SINK_HPP
#define LOG_HISTORY_SINK_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "spdlog/sinks/base_sink.h"

namespace neon
{
  /// Keeps the last lines that were logged, formatted as the console shows
  /// them, for a window that shows the log while the application runs.
  /// The oldest line goes when there are more than `most`. Nothing is
  /// written anywhere; the other sinks do that.
  class LogHistorySink final : public spdlog::sinks::base_sink<std::mutex>
  {
    std::size_t _most;
    std::deque<std::string> _lines;
    std::uint64_t _revision = 0;

  protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
      spdlog::memory_buf_t formatted;
      formatter_->format(msg, formatted);

      // the formatter ends every line with a newline, which a window adds
      // itself
      std::string line(formatted.data(), formatted.size());
      while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) { line.pop_back(); }

      _lines.push_back(std::move(line));
      while (_lines.size() > _most) { _lines.pop_front(); }
      _revision++;
    }

    void flush_() override {}

  public:
    /// How many lines are kept unless the sink is told otherwise.
    static constexpr std::size_t default_most = 1000;

    explicit LogHistorySink(const std::size_t most = default_most)
    {
      _most = most;
    }

    /// Copies the lines that are kept, the oldest first, in place of what
    /// `lines` held.
    void CopyLines(std::vector<std::string> &lines)
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      lines.assign(_lines.begin(), _lines.end());
    }

    /// Goes up with every line that is logged, so that a window knows when
    /// to read the lines again.
    [[nodiscard]] std::uint64_t Revision()
    {
      const std::lock_guard<std::mutex> lock(mutex_);
      return _revision;
    }
  };
} // neon

#endif //LOG_HISTORY_SINK_HPP
