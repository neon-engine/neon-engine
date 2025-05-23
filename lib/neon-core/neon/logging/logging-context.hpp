#ifndef LOGGING_CONTEXT_HPP
#define LOGGING_CONTEXT_HPP

#include <memory>

#include "logger.hpp"

namespace neon
{
  class LoggingContext
  {
  protected:
    ~LoggingContext() = default;

  public:
    virtual std::shared_ptr<Logger> CreateLogger(const std::string &name) = 0;
  };
} // neon

#endif //LOGGING_CONTEXT_HPP
