#ifndef ERROR_COUNT_SINK_HPP
#define ERROR_COUNT_SINK_HPP

#include <atomic>
#include <cstddef>
#include <mutex>

#include "spdlog/sinks/base_sink.h"

namespace neon
{
  /// Counts the messages that were logged at Error or above. A run that
  /// goes on after a problem, as a game does, still has to say at its end
  /// that something went wrong: the runtime turns this count into its exit
  /// code. Nothing is written anywhere; the other sinks do that.
  class ErrorCountSink final : public spdlog::sinks::base_sink<std::mutex>
  {
    std::atomic<std::size_t> _errors = 0;

  protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
      if (msg.level >= spdlog::level::err) { _errors++; }
    }

    void flush_() override {}

  public:
    [[nodiscard]] std::size_t Count() const
    {
      return _errors.load();
    }
  };
} // neon

#endif //ERROR_COUNT_SINK_HPP
