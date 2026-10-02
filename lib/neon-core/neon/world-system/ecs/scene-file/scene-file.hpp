#ifndef SCENE_FILE_HPP
#define SCENE_FILE_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/world-system/ecs/scene.hpp>

#include "component-format.hpp"

namespace neon
{
  /// A scene that is kept in a file.
  ///
  /// The file lists entities. An entity has a name, the components it
  /// carries, and the entities below it:
  ///
  ///     scene: demo
  ///     version: 1
  ///
  ///     entities:
  ///       - name: player
  ///         components:
  ///           Transform:
  ///             position: [0, 0, 2]
  ///           Spectator: Default
  ///         children:
  ///           - name: camera
  ///             components:
  ///               Transform: Default
  ///               Camera: Default
  ///
  /// What a component leaves out keeps its default, and `Default` keeps all of
  /// them. `{}` means the same. A name that is not known
  /// is an error, so that a name that was misspelled does not go unnoticed.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed
  /// in. The file is read and written through the file system.
  class SceneFile final : public Scene
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::string _path;
    ComponentFormats _component_formats;

    bool Read(EntityStore &store);
    std::shared_ptr<Logger> _logger;

    void ReadEntity(
      const DataValue &value,
      const std::string &label,
      EntityStore &store,
      Entity parent,
      std::vector<std::string> &errors) const;

    [[nodiscard]] DataValue WriteEntity(EntityStore &store, Entity entity) const;

  public:
    /// The version of the layout of the file that is written, and the
    /// highest that is read.
    static constexpr int version = 1;

    /// `path` is a virtual path, such as `assets://scenes/demo.scene.yml`.
    SceneFile(
      FileSystemContext *file_system,
      DocumentFormat *format,
      const std::string &path,
      const std::shared_ptr<Logger> &logger);

    /// The kinds of components the file can hold. Those of the engine are
    /// known. A game adds its own.
    [[nodiscard]] ComponentFormats &GetComponentFormats();

    /// Reads the file and creates its entities. Returns false when the file
    /// cannot be read or something in it is wrong; what could be read is in
    /// the store, and every problem that was found is logged with its line,
    /// not only the first, so that a file is corrected in one pass.
    bool Populate(EntityStore &store) override;

    /// Reads another scene file in place of the one the world started with,
    /// into a store the world has emptied. The path becomes the file's path,
    /// so that Save() and messages name the scene that is shown.
    bool Load(EntityStore &store, const std::string &path) override;

    /// The virtual path of the scene that was read last.
    [[nodiscard]] const std::string &GetPath() const;

    /// Writes every entity of the store to a file, as a scene of the given
    /// name. Components without a format are left out. Returns false when
    /// the file cannot be written.
    bool Save(EntityStore &store, const std::string &name, const std::string &path) const;
  };
} // neon

#endif //SCENE_FILE_HPP
