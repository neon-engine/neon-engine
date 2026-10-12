#ifndef PROCESS_COMMAND_HPP
#define PROCESS_COMMAND_HPP

#include <string>
#include <vector>

namespace neon
{
  /// A program to start as a process of its own, with what it is given.
  ///
  /// The program is a name that is looked for on the PATH of the
  /// environment, such as `cmake`, or a native path to the executable. The
  /// arguments are handed to it one by one, as they are, so nothing in them
  /// is read by a shell: a space is part of an argument, and a quote is a
  /// character like any other. A shell is started on purpose, by naming it as
  /// the program.
  ///
  /// The native paths here are what the person who set up the machine
  /// wrote, as in the toolchain files of the editor, and go to the
  /// ProcessContext alone.
  struct ProcessCommand
  {
    std::string program;
    std::vector<std::string> arguments;

    /// The folder the program starts in, as a native path. Empty is the
    /// folder of the application that starts it.
    std::string directory;
  };
} // neon

#endif //PROCESS_COMMAND_HPP
