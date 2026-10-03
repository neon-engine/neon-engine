#ifndef MOCK_LIBRARY_LOADER_HPP
#define MOCK_LIBRARY_LOADER_HPP

#include <memory>
#include <string>

#include <gmock/gmock.h>

#include <neon/extension/library-loader.hpp>

namespace neon::testing
{
  class MockLibraryLoader : public LibraryLoader
  {
  public:
    MOCK_METHOD(std::string, GetPlatform, (), (const, override));
    MOCK_METHOD(std::unique_ptr<NativeLibrary>, Open, (const std::string &path, std::string &error), (override));
  };
} // neon::testing

#endif //MOCK_LIBRARY_LOADER_HPP
