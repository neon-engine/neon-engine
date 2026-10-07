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
#include "prefab-files.hpp"

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
  ///       - name: north-wall
  ///         prefab: assets://prefabs/wall.prefab.yml
  ///         components:
  ///           Transform:
  ///             position: [0, 0, -3]
  ///
  /// What a component leaves out keeps its default, and `Default` keeps all of
  /// them. `{}` means the same. A name that is not known
  /// is an error, so that a name that was misspelled does not go unnoticed.
  ///
  /// An entity with a `prefab` starts as the entity of that prefab recipe,
  /// see PrefabFile, and what is written next to it goes on top: a
  /// component is read into the one the prefab gave, so that only the
  /// values that are written change, `~` takes a component away, and a
  /// child of the same name as one of the prefab's is read onto it. Every
  /// such entity carries a Prefab component that names the file.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed
  /// in. The file is read and written through the file system.
  class SceneFile final : public Scene
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::string _path;
    ComponentFormats _component_formats;
    std::shared_ptr<Logger> _logger;

    // the prefabs the scene placed, kept for the life of the scene so that
    // a spawn does not read its file again, and forgotten by the next load
    PrefabFiles _prefabs;

    // whether the last read found no document: no file, or no YAML
    bool _could_not_be_read = false;

    // the document ReadAhead() read, and its path, until Load() places it
    std::string _read_ahead_path;
    DataValue _read_ahead;

    bool Read(EntityStore &store);

    /// Reads the file at `path` into a document. Returns false when there
    /// is none: a file that is not there or cannot be opened is said in the
    /// log, and a text that is no document is added to `errors`.
    bool ReadDocument(const std::string &path, DataValue &document, std::vector<std::string> &errors) const;

    /// Logs every problem of a read, with its line, and a last line that
    /// counts them: `<what> has N problems, <goes_on>`.
    void ReportProblems(
      const std::vector<std::string> &errors,
      const std::string &what,
      const std::string &goes_on) const;

    /// Reads a list of entities below a parent. `of` names the parent in
    /// messages, and is empty at the top. When `onto_existing` is set, an
    /// entity whose name is there already is read onto that entity, which
    /// is how a scene changes the children of a prefab; otherwise such a
    /// name is reported. A name twice in the list itself is always
    /// reported.
    void ReadEntities(
      const DataValue &list,
      const std::string &document,
      const std::string &of,
      EntityStore &store,
      Entity parent,
      bool onto_existing,
      PrefabFiles &prefabs,
      std::vector<std::string> &errors) const;

    /// Takes a child that a prefab gave away, written as `- shade: ~` among
    /// the children. `item` is what was written, for its line. A child that
    /// is not there, or one not below a prefab, is reported.
    void TakeAwayChild(
      const DataValue &item,
      const std::string &name,
      const std::string &document,
      const std::string &of,
      EntityStore &store,
      Entity parent,
      bool onto_existing,
      std::vector<std::string> &errors) const;

    void ReadEntity(
      const DataValue &value,
      const std::string &document,
      const std::string &label,
      EntityStore &store,
      Entity parent,
      bool onto_existing,
      PrefabFiles &prefabs,
      std::vector<std::string> &errors) const;

    /// Reads what is written for an entity onto it: its prefab first, then
    /// its components, then its children. `reader` reads `value`, and says
    /// where the problems go.
    void ReadOnto(
      const DataReader &reader,
      const DataValue &value,
      EntityStore &store,
      Entity entity,
      bool onto_existing,
      PrefabFiles &prefabs) const;

    /// Places the prefab at a path onto an entity. `written` is the value
    /// that names it, for the line of a message. Returns false when nothing
    /// was placed, which was reported.
    bool PlacePrefab(
      const std::string &path,
      const DataValue &written,
      const DataReader &reader,
      EntityStore &store,
      Entity entity,
      bool onto_existing,
      PrefabFiles &prefabs) const;

    void ReadComponents(
      const DataReader &reader,
      const DataValue &value,
      EntityStore &store,
      Entity entity) const;

    /// Reads one component onto the entity. `components` reads the map the
    /// component was written in, and says where a problem goes; `where`
    /// names the entity in a message.
    void ReadComponent(
      const DataReader &components,
      const std::string &where,
      const std::string &name,
      const DataValue &component,
      EntityStore &store,
      Entity entity) const;

    /// The names of the components the file can hold, for a message.
    [[nodiscard]] std::string KnownComponents() const;

    /// The names of the children of a prefab's entity, those of the prefab
    /// it starts from included, for the children a scene took away.
    void ChildrenOfPrefab(const std::string &path, std::vector<std::string> &names, std::vector<std::string> &seen);

    [[nodiscard]] DataValue WriteEntity(EntityStore &store, Entity entity);

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
    /// not only the first, so that a file is corrected in one pass. The
    /// prefabs the scene places are read once each, and the Prefab
    /// component is registered with the store.
    bool Populate(EntityStore &store) override;

    /// Reads the file of another scene and keeps its document for Load(),
    /// so that the file is read once. Says why when it cannot be read at
    /// all.
    bool ReadAhead(const std::string &path) override;

    /// Reads another scene file in place of the one the world started with,
    /// into a store the world has emptied, or places what ReadAhead() read
    /// of it. The path becomes the file's path, so that Save() and messages
    /// name the scene that is shown.
    bool Load(EntityStore &store, const std::string &path) override;

    /// Whether the last read found no file there, could not open it, or
    /// found no document in it.
    [[nodiscard]] bool CouldNotBeRead() const override;

    /// Places a prefab below `parent` while the game plays, through the
    /// same reading a scene file goes through, so that a spawned entity is
    /// what a placed one is: it carries the Prefab component too. The
    /// prefab is read from its file once for the life of the scene. See
    /// Scene::Spawn for `overrides` and what is returned.
    Entity Spawn(EntityStore &store, const std::string &path, Entity parent, const DataValue &overrides) override;

    /// The virtual path of the scene that was read last.
    [[nodiscard]] const std::string &GetPath() const;

    /// Writes every entity of the store to a file, as a scene of the given
    /// name. Components without a format are left out. An entity that was
    /// placed from a prefab is written with its `prefab` and with every
    /// component it has, not only what differs from the prefab, and a child
    /// of the prefab it does not have is written as taken away. Returns
    /// false when the file cannot be written.
    bool Save(EntityStore &store, const std::string &name, const std::string &path);
  };
} // neon

#endif //SCENE_FILE_HPP
