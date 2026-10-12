#include "os-process-runner.hpp"

#include <array>
#include <cerrno>
#include <cstring>
#include <format>
#include <vector>

#if defined(_WIN32)
#include <windows.h>

#include <neon/filesystem/native-path.hpp>
#else
#include <fcntl.h>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
#endif

namespace neon
{
  // Helpers of OS_ProcessRunner: the lines out of what a pipe delivers, and
  // the pieces of each platform that have no counterpart on the other.
  namespace
  {
    /// Cuts the bytes a pipe delivered into lines for the callback, and
    /// keeps what comes after the last line break for the next delivery.
    class LineCutter final
    {
      std::string _rest;
      const ProcessContext::LineCallback &_on_line;

    public:
      explicit LineCutter(const ProcessContext::LineCallback &on_line)
        : _on_line(on_line) {}

      void Add(const char *bytes, const std::size_t count)
      {
        _rest.append(bytes, count);

        std::size_t start = 0;
        for (std::size_t end = _rest.find('\n'); end != std::string::npos; end = _rest.find('\n', start))
        {
          std::size_t length = end - start;
          // a line break of Windows ends with the same character
          if (length > 0 && _rest[start + length - 1] == '\r') { length--; }
          _on_line(_rest.substr(start, length));
          start = end + 1;
        }
        _rest.erase(0, start);
      }

      /// What was printed after the last line break, when the process
      /// ended without one.
      void Finish()
      {
        if (!_rest.empty())
        {
          if (_rest.back() == '\r') { _rest.pop_back(); }
          if (!_rest.empty()) { _on_line(_rest); }
          _rest.clear();
        }
      }
    };

#if defined(_WIN32)
    std::wstring wide_of(const std::string &utf8)
    {
      return NativePath::FromUtf8(utf8).wstring();
    }

    /// One argument as the command line of Windows carries it, so that the
    /// program reads it back as it was given: in quotes when it holds a
    /// space or a quote, with the backslashes in front of a quote doubled,
    /// which is how the C runtime takes a command line apart.
    void append_quoted(std::wstring &line, const std::wstring &argument)
    {
      if (!argument.empty() && argument.find_first_of(L" \t\"") == std::wstring::npos)
      {
        line += argument;
        return;
      }

      line += L'"';
      std::size_t backslashes = 0;
      for (const wchar_t character : argument)
      {
        if (character == L'\\')
        {
          backslashes++;
          continue;
        }
        if (character == L'"')
        {
          line.append(backslashes * 2 + 1, L'\\');
          backslashes = 0;
          line += L'"';
          continue;
        }
        line.append(backslashes, L'\\');
        backslashes = 0;
        line += character;
      }
      line.append(backslashes * 2, L'\\');
      line += L'"';
    }

    std::string last_error_text()
    {
      const DWORD code = GetLastError();
      wchar_t *text = nullptr;
      const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, code, 0, reinterpret_cast<LPWSTR>(&text), 0, nullptr);
      std::string message = std::format("error {}", code);
      if (length > 0 && text != nullptr)
      {
        message = NativePath::ToUtf8(std::wstring(text, length));
        while (!message.empty() && (message.back() == '\r' || message.back() == '\n' || message.back() == ' '))
        {
          message.pop_back();
        }
      }
      if (text != nullptr) { LocalFree(text); }
      return message;
    }
#else
    bool is_program(const std::string &native_path)
    {
      struct stat status{};
      if (stat(native_path.c_str(), &status) != 0) { return false; }
      return S_ISREG(status.st_mode) && access(native_path.c_str(), X_OK) == 0;
    }

    /// The folders of PATH, in order.
    std::vector<std::string> path_folders()
    {
      std::vector<std::string> folders;
      const char *path = getenv("PATH");
      if (path == nullptr) { return folders; }

      const std::string all(path);
      std::size_t start = 0;
      while (start <= all.size())
      {
        const std::size_t end = all.find(':', start);
        const std::string folder = all.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!folder.empty()) { folders.push_back(folder); }
        if (end == std::string::npos) { break; }
        start = end + 1;
      }
      return folders;
    }
#endif
  }

#if defined(_WIN32)
  bool OS_ProcessRunner::Find(const std::string &program, std::string &native_path)
  {
    // the search of Windows: a path as it is, a name in the folder of the
    // application, the current folder, the system folders, and PATH, with
    // .exe added when the name has no extension
    const std::wstring name = wide_of(program);
    std::array<wchar_t, MAX_PATH> found{};
    const DWORD length = SearchPathW(nullptr, name.c_str(), L".exe", static_cast<DWORD>(found.size()), found.data(), nullptr);
    if (length == 0 || length >= found.size()) { return false; }

    native_path = NativePath::ToUtf8(std::wstring(found.data(), length));
    return true;
  }

  bool OS_ProcessRunner::Run(
    const ProcessCommand &command,
    const LineCallback &on_line,
    int &exit_code,
    std::string &error)
  {
    std::string program;
    if (!Find(command.program, program))
    {
      error = std::format("{} is not installed: it is not on PATH, and no file of that name is there", command.program);
      return false;
    }

    std::wstring line;
    append_quoted(line, wide_of(program));
    for (const auto &argument : command.arguments)
    {
      line += L' ';
      append_quoted(line, wide_of(argument));
    }

    // one pipe for both outputs, whose reading end the child does not inherit
    SECURITY_ATTRIBUTES inheritable{};
    inheritable.nLength = sizeof(inheritable);
    inheritable.bInheritHandle = TRUE;
    HANDLE read_end = nullptr;
    HANDLE write_end = nullptr;
    if (!CreatePipe(&read_end, &write_end, &inheritable, 0))
    {
      error = std::format("A pipe for the output of {} could not be made: {}", command.program, last_error_text());
      return false;
    }
    SetHandleInformation(read_end, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = write_end;
    startup.hStdError = write_end;

    const std::wstring directory = wide_of(command.directory);
    const std::wstring application = wide_of(program);
    PROCESS_INFORMATION process{};
    const BOOL started = CreateProcessW(
      application.c_str(),
      line.data(),
      nullptr,
      nullptr,
      TRUE,
      CREATE_NO_WINDOW,
      nullptr,
      directory.empty() ? nullptr : directory.c_str(),
      &startup,
      &process);
    CloseHandle(write_end);

    if (!started)
    {
      error = std::format("{} could not be started: {}", command.program, last_error_text());
      CloseHandle(read_end);
      return false;
    }
    CloseHandle(process.hThread);

    LineCutter cutter(on_line);
    std::array<char, 4096> buffer{};
    DWORD count = 0;
    while (ReadFile(read_end, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr) && count > 0)
    {
      cutter.Add(buffer.data(), count);
    }
    cutter.Finish();
    CloseHandle(read_end);

    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess);

    exit_code = static_cast<int>(code);
    return true;
  }
#else
  bool OS_ProcessRunner::Find(const std::string &program, std::string &native_path)
  {
    if (program.empty()) { return false; }

    // a path is taken as it is, as a shell takes one with a slash in it
    if (program.find('/') != std::string::npos)
    {
      if (!is_program(program)) { return false; }
      native_path = program;
      return true;
    }

    for (const auto &folder : path_folders())
    {
      const std::string candidate = folder + "/" + program;
      if (is_program(candidate))
      {
        native_path = candidate;
        return true;
      }
    }
    return false;
  }

  bool OS_ProcessRunner::Run(
    const ProcessCommand &command,
    const LineCallback &on_line,
    int &exit_code,
    std::string &error)
  {
    std::string program;
    if (!Find(command.program, program))
    {
      error = std::format("{} is not installed: it is not on PATH, and no file of that name is there", command.program);
      return false;
    }

    std::array<int, 2> pipe_ends{};
    if (pipe(pipe_ends.data()) != 0)
    {
      error = std::format("A pipe for the output of {} could not be made: {}", command.program, std::strerror(errno));
      return false;
    }
    const int read_end = pipe_ends[0];
    const int write_end = pipe_ends[1];
    fcntl(read_end, F_SETFD, FD_CLOEXEC);

    // the child writes both of its outputs into the pipe and holds no end
    // of it otherwise, so that the pipe ends when the child does
    posix_spawn_file_actions_t actions{};
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, write_end, STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&actions, write_end, STDERR_FILENO);
    posix_spawn_file_actions_addclose(&actions, write_end);
    if (!command.directory.empty())
    {
      posix_spawn_file_actions_addchdir_np(&actions, command.directory.c_str());
    }

    std::vector<std::string> words;
    words.reserve(command.arguments.size() + 1);
    words.push_back(program);
    words.insert(words.end(), command.arguments.begin(), command.arguments.end());
    std::vector<char *> argv;
    argv.reserve(words.size() + 1);
    for (auto &word : words) { argv.push_back(word.data()); }
    argv.push_back(nullptr);

    pid_t child = 0;
    const int spawned = posix_spawn(&child, program.c_str(), &actions, nullptr, argv.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    close(write_end);

    if (spawned != 0)
    {
      error = std::format("{} could not be started: {}", command.program, std::strerror(spawned));
      close(read_end);
      return false;
    }

    LineCutter cutter(on_line);
    std::array<char, 4096> buffer{};
    while (true)
    {
      const ssize_t count = read(read_end, buffer.data(), buffer.size());
      if (count > 0)
      {
        cutter.Add(buffer.data(), static_cast<std::size_t>(count));
        continue;
      }
      if (count < 0 && errno == EINTR) { continue; }
      break;
    }
    cutter.Finish();
    close(read_end);

    int status = 0;
    while (waitpid(child, &status, 0) < 0)
    {
      if (errno != EINTR)
      {
        error = std::format("The end of {} could not be waited for: {}", command.program, std::strerror(errno));
        return false;
      }
    }

    if (WIFEXITED(status))
    {
      exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status))
    {
      exit_code = 128 + WTERMSIG(status);
    } else
    {
      exit_code = -1;
    }
    return true;
  }
#endif
} // neon
