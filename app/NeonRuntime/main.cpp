#include <neon/runtime/runtime-composition.hpp>

#include "runtime-hooks.hpp"

// What the platform starts the application by, which calls the main() below.
// It is the window backend's, and this is its header for it.
#include <neon/application/sdl2-main.hpp>

// The composition root of NeonRuntime, and nothing more: the start-up that
// every application shares, RuntimeComposition of lib/neon-runtime, with the
// runtime's own choices in RuntimeHooks. The runtime plays the project that
// is put together next to it, and a player gets it. See
// docs/development.md, "How an application starts".
int main(const int argc, char *argv[])
{
  RuntimeHooks hooks;
  neon::RuntimeComposition composition(
    neon::ApplicationDescription{
      .program = "NeonRuntime",
      .summary = "Runs a Neon Engine project.",
      .in_prose = "the runtime"
    },
    &hooks);

  if (composition.ParseCommandLine(argc, argv) &&
      composition.OpenFileSystem() &&
      composition.ReadProject() &&
      composition.PlaceUserFolder() &&
      composition.ReadSettings() &&
      composition.StartExtensions() &&
      composition.CreateSystems())
  {
    composition.Run();
  }

  return composition.CleanUp();
}
