#ifndef DEFERRED_FILE_SINK_HPP
#define DEFERRED_FILE_SINK_HPP

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "spdlog/details/log_msg_buffer.h"
#include "spdlog/sinks/base_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

namespace neon
{
  /// The sink of a log file whose place is only known after logging has
  /// started, because the file system that knows it starts later.
  ///
  /// Until the file is opened, it holds on to every message. Opening the file
  /// writes them first, in the order and with the times they were logged at,
  /// so that the file tells the whole run. A run that never gets a file lets
  /// them go with GoWithout().
  class DeferredFileSink final : public spdlog::sinks::base_sink<std::mutex>
  {
    std::size_t _max_size;
    std::size_t _max_files;
    std::size_t _max_held;

    bool _holding = true;
    std::vector<spdlog::details::log_msg_buffer> _held;
    std::size_t _dropped = 0;

    std::unique_ptr<spdlog::sinks::rotating_file_sink_st> _file;

  public:
    /// The number of messages held by default. A start-up logs far fewer.
    /// The limit keeps a run whose file is never opened from growing forever.
    static constexpr std::size_t default_max_held = 4096;

    /// `max_size` and `max_files` are those of a rotating file sink: the
    /// size at which the file is moved aside, and how many such files are
    /// kept.
    DeferredFileSink(std::size_t max_size, std::size_t max_files, std::size_t max_held = default_max_held);

    /// Opens the file at a native path, adding to it if it exists, and writes
    /// what was held into it. A file that was open before is closed.
    ///
    /// Throws spdlog::spdlog_ex if the file cannot be opened. What was held
    /// is then let go, and the sink writes nothing from then on.
    void Open(const std::string &native_path);

    /// Stops holding on to messages, and lets go of those it has. Nothing is
    /// written from then on, unless a file is opened after all.
    void GoWithout();

    /// Whether a file is open.
    [[nodiscard]] bool IsOpen();

  protected:
    void sink_it_(const spdlog::details::log_msg &msg) override;

    void flush_() override;

    void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter) override;
  };
} // neon

#endif //DEFERRED_FILE_SINK_HPP
