// An extension in C++ that keeps something of its own between being started
// and being cleaned up, and says goodbye through the application it was
// handed.

#include <string>

#include <neon/extension/neon-extension.h>

// What the extension keeps, for this file alone.
namespace
{
  struct Polite
  {
    const NeonExtensionHost *host = nullptr;
    std::string farewell;
  };

  // one for the library, which is handed back as the context
  Polite polite;

  void clean_up(void *context)
  {
    const auto *kept = static_cast<const Polite *>(context);
    kept->host->log(kept->host->context, NEON_LOG_INFO, kept->farewell.c_str());
  }
}

extern "C" NEON_EXTENSION_EXPORT int neon_extension_initialize(
  const NeonExtensionHost *host,
  NeonExtension *extension)
{
  polite.host = host;
  polite.farewell = "Goodbye from an extension in C++";

  extension->abi_version = NEON_EXTENSION_ABI_VERSION;
  extension->context = &polite;
  extension->clean_up = &clean_up;

  host->log(host->context, NEON_LOG_WARN, "Hello from an extension in C++");
  return 1;
}
