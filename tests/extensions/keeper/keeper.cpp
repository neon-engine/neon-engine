// An extension that keeps what is the player's: it saves games under
// user://, finds them again, and asks the application to close.

#include <cstdint>
#include <string>
#include <vector>

#include <neon/extension/neon-extension.hpp>

namespace
{
  using neon::extension::World;

  std::vector<std::uint8_t> bytes_of(const std::string &text)
  {
    return {text.begin(), text.end()};
  }

  /// Saves two games when it starts, lists and reads them, tries to write
  /// where it may not, and asks to close when `quit` is pressed.
  class Keeping final : public neon::extension::System
  {
  public:
    void Start(World &world) override
    {
      // a folder that is not there yet is created, and one below it is
      // left out of what is listed
      const bool saved = world.WriteFile("user://saves/slot-2.sav", bytes_of("ogres: 1"))
                         && world.WriteFile("user://saves/slot-1.sav", bytes_of("a longer game that is replaced"))
                         && world.WriteFile("user://saves/slot-1.sav", bytes_of("shamblers: 2"))
                         && world.WriteFile("user://saves/old/slot-0.sav", {})
                         && world.WriteFile("user://saves/archive/2026/slot-3.sav", {});

      std::string names;
      for (const auto &name : world.ListFiles("user://saves")) { names += (names.empty() ? "" : ", ") + name; }

      // the folders, and one that holds only a folder among them
      std::string folders;
      for (const auto &name : world.ListFolders("user://saves/")) { folders += (folders.empty() ? "" : ", ") + name; }
      world.Info("The folders of the saves are " + folders + ", and a folder that is not there holds "
                 + std::to_string(world.ListFolders("user://nothing").size()) + " folders");

      std::vector<std::uint8_t> first;
      const bool read = world.ReadFile("user://saves/slot-1.sav", first);
      world.Info(std::string("Saved: ") + (saved ? "yes" : "no") + ", found " + names + ", and read back: "
                 + (read ? std::string(first.begin(), first.end()) : "nothing"));

      // what is not the player's, what climbs out of it, and what has no name
      const bool refused = !world.WriteFile("assets://saves/slot-1.sav", bytes_of("no"))
                           && !world.WriteFile("output://slot-1.sav", bytes_of("no"))
                           && !world.WriteFile("saves/slot-1.sav", bytes_of("no"))
                           && !world.WriteFile("user://../outside.sav", bytes_of("no"))
                           && !world.WriteFile("user://saves/../../outside.sav", bytes_of("no"))
                           && !world.WriteFile("", bytes_of("no"));
      world.Info(std::string("What is not the player's was refused: ") + (refused ? "yes" : "no")
                 + ", and a folder that is not there holds "
                 + std::to_string(world.ListFiles("user://nothing").size()) + " files");

      // what ships with the application is listed as well
      const auto own = world.ListFiles("extensions://keeper/");
      world.Info("Its own folder holds " + std::to_string(own.size()) + " files, the first " + (own.empty() ? "" : own[0]));
    }

    void Update(World &world, const double delta_time) override
    {
      if (world.WasActionPressed("quit")) { world.RequestQuit(); }
    }
  };

  class KeeperExtension final : public neon::extension::Extension
  {
  public:
    bool Initialize(World &world) override
    {
      AddSystem<Keeping>("Keeping");
      return true;
    }
  };
}

NEON_EXTENSION(KeeperExtension)
