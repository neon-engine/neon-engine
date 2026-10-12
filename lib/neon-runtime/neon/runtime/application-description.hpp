#ifndef APPLICATION_DESCRIPTION_HPP
#define APPLICATION_DESCRIPTION_HPP

#include <string>

namespace neon
{
  /// How an application names itself where the shared start-up speaks of it:
  /// on its command line, in its log, and in what it says on the console.
  struct ApplicationDescription
  {
    /// The name of the executable, as the help text and the log name it:
    /// `NeonRuntime`.
    std::string program;

    /// One line for the help text, what the application does: "Runs a Neon
    /// Engine project."
    std::string summary;

    /// How a sentence on the console refers to it: "the runtime", as in "is
    /// not next to the runtime".
    std::string in_prose;
  };
} // neon

#endif //APPLICATION_DESCRIPTION_HPP
