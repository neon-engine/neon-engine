#ifndef MOCK_FILE_SYSTEM_CONTEXT_HPP
#define MOCK_FILE_SYSTEM_CONTEXT_HPP

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <neon/filesystem/file-system-context.hpp>

namespace neon::testing
{
  class MockFileSystemContext : public FileSystemContext
  {
  public:
    MOCK_METHOD(bool, Exists, (const std::string &path), (override));

    MOCK_METHOD(bool, ReadBytes, (const std::string &path, std::vector<unsigned char> &contents), (override));

    MOCK_METHOD(bool, ReadText, (const std::string &path, std::string &contents), (override));

    MOCK_METHOD(bool, WriteBytes, (const std::string &path, const std::vector<unsigned char> &contents), (override));

    MOCK_METHOD(bool, WriteText, (const std::string &path, const std::string &contents), (override));

    MOCK_METHOD(bool, ListFiles, (const std::string &directory, std::vector<std::string> &paths), (override));
  };
} // neon::testing

#endif //MOCK_FILE_SYSTEM_CONTEXT_HPP
