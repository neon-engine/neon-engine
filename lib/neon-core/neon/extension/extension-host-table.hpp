#ifndef EXTENSION_HOST_TABLE_HPP
#define EXTENSION_HOST_TABLE_HPP

#include "loaded-extension.hpp"

namespace neon
{
  /// Fills in what the application offers an extension, the
  /// NeonExtensionHost of neon-extension.h, with the functions of the
  /// engine behind it. The context of the table is the extension itself,
  /// so that every call knows who made it.
  void FillExtensionHostTable(LoadedExtension &extension);
} // neon

#endif //EXTENSION_HOST_TABLE_HPP
