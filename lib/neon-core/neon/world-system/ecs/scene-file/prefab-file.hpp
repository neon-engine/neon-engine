#ifndef PREFAB_FILE_HPP
#define PREFAB_FILE_HPP

#include <string>
#include <vector>

#include <neon/data/data-value.hpp>
#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>

namespace neon
{
  /// A prefab recipe: an entity, with its components and the entities below
  /// it, described once in a file of its own and placed in scenes.
  ///
  ///     version: 1
  ///     prefab: wall
  ///
  ///     entity:
  ///       components:
  ///         Renderable:
  ///           model: assets://external/kenney/prototype-kit/wall.glb
  ///           shader: engine://shaders/basic-lit
  ///         RigidBody:
  ///           kind: static
  ///       children:
  ///         - name: post
  ///           components:
  ///             Collider: Default
  ///
  /// The entity has no name of its own: it is called what the scene calls
  /// it where it is placed. The file is read once and its entity is kept as
  /// a tree of values, which SceneFile reads onto every entity the prefab
  /// is placed as, so that the components are read through the same formats
  /// a scene uses. Which format the file has is up to the DocumentFormat
  /// that is handed in.
  class PrefabFile final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::string _path;
    std::string _name;
    DataValue _entity;

  public:
    /// The version of the layout of the file that is read, and the highest
    /// that is understood.
    static constexpr int version = 1;

    /// `path` is a virtual path, such as `assets://prefabs/wall.prefab.yml`.
    PrefabFile(FileSystemContext *file_system, DocumentFormat *format, const std::string &path);

    /// Reads the file. Returns false when it cannot be read or is no
    /// document, with the reason in `errors` where it has a line. A name at
    /// the top that is not known, a version this engine does not read, and
    /// a missing entity are in `errors` with their line too, and the entity
    /// holds what could be read. The file is read once; a second call reads
    /// it again.
    bool Read(std::vector<std::string> &errors);

    [[nodiscard]] const std::string &GetPath() const;

    /// What the prefab is called, from `prefab` at the top. Empty when the
    /// file does not say.
    [[nodiscard]] const std::string &GetName() const;

    /// The entity the prefab describes, as it was written: `prefab`,
    /// `components`, and `children`. Nothing when the file holds none.
    [[nodiscard]] const DataValue &GetEntity() const;
  };
} // neon

#endif //PREFAB_FILE_HPP
