#include <neon/runtime/runtime-composition.hpp>

#include "editor-hooks.hpp"

// What the platform starts the application by, which calls the main() below.
// It is the window backend's, and this is its header for it.
#include <neon/application/sdl2-main.hpp>

// The composition root of NeonEditor, and nothing more: the start-up that
// every application shares, RuntimeComposition of lib/neon-runtime, with the
// editor's own choices in EditorHooks. The editor opens the project folder
// it is given, where the runtime runs the project next to it; it registers
// the editor's options next to the runtime's; and with a window it draws
// the tool interface, where without one it runs as the runtime does. See
// docs/editor.md.
int main(const int argc, char *argv[])
{
  neon::EditorHooks hooks;
  neon::RuntimeComposition composition(
    neon::ApplicationDescription{
      .program = "NeonEditor",
      .summary = "Edits and runs a Neon Engine project.",
      .in_prose = "the editor"
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
