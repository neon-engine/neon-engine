#include "deferred-file-sink.hpp"

#include <string_view>

namespace neon
{
  DeferredFileSink::DeferredFileSink(const std::size_t max_size, const std::size_t max_files, const std::size_t max_held)
  {
    _max_size = max_size;
    _max_files = max_files;
    _max_held = max_held;
  }

  void DeferredFileSink::Open(const std::string &native_path)
  {
    std::lock_guard lock(mutex_);

    _file.reset();

    try
    {
      _file = std::make_unique<spdlog::sinks::rotating_file_sink_st>(native_path, _max_size, _max_files);
    } catch (...)
    {
      // nothing will ever read what was held, so it is let go
      _holding = false;
      _held.clear();
      _held.shrink_to_fit();
      _dropped = 0;
      throw;
    }

    // the file writes its lines the way this sink was told to
    _file->set_formatter(formatter_->clone());

    for (const auto &message : _held)
    {
      _file->log(message);
    }

    if (_dropped > 0)
    {
      const std::string text = std::to_string(_dropped) +
                               " messages that were logged before the log file was opened are missing here. " +
                               "They went to the console only";
      const spdlog::details::log_msg note(std::string_view("LoggingSystem"), spdlog::level::warn, text);
      _file->log(note);
    }

    _holding = false;
    _held.clear();
    _held.shrink_to_fit();
    _dropped = 0;
  }

  void DeferredFileSink::GoWithout()
  {
    std::lock_guard lock(mutex_);

    _holding = false;
    _held.clear();
    _held.shrink_to_fit();
    _dropped = 0;
  }

  bool DeferredFileSink::IsOpen()
  {
    std::lock_guard lock(mutex_);
    return _file != nullptr;
  }

  void DeferredFileSink::sink_it_(const spdlog::details::log_msg &msg)
  {
    if (_file != nullptr)
    {
      _file->log(msg);
      return;
    }

    if (!_holding) { return; }

    // the message points at text that is gone once this returns, so a copy
    // that owns it is kept
    if (_held.size() < _max_held)
    {
      _held.emplace_back(msg);
    } else
    {
      _dropped++;
    }
  }

  void DeferredFileSink::flush_()
  {
    if (_file != nullptr) { _file->flush(); }
  }

  void DeferredFileSink::set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter)
  {
    if (_file != nullptr) { _file->set_formatter(sink_formatter->clone()); }
    base_sink::set_formatter_(std::move(sink_formatter));
  }
} // neon
