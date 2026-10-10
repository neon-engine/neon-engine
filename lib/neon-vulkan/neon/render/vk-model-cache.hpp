#ifndef VK_MODEL_CACHE_HPP
#define VK_MODEL_CACHE_HPP

#include <cstddef>
#include <memory>
#include <set>
#include <neon/common/data-buffer.hpp>
#include <neon/common/shared-ids.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/model-key.hpp>
#include <neon/render/render-info.hpp>

#include "vk-device.hpp"
#include "vk-model.hpp"

namespace neon
{
  /// The models the render objects draw, held once each.
  ///
  /// A model is held once for every ModelKey, however many render objects
  /// draw it: a file by its path and fit, whatever its format, and a mesh
  /// that was built by its key, the values it was built from. The first
  /// render object loads or uploads it, the others take a reference, and it
  /// is freed when the last one releases it. A mesh that was built without
  /// a key belongs to its render object alone, and is neither looked up nor
  /// shared.
  ///
  /// A model is known by an id that stays the same while it is held, which
  /// a render object keeps as its model id.
  ///
  /// A file that cannot be read, by its path and fit, is remembered as
  /// failed for the life of the cache, as the physics remembers it: it is
  /// read and logged once, and every render object that names it after
  /// that draws nothing and says nothing. A scene that takes the place of
  /// another does not forget it, since the cache keeps nothing by scene.
  // ReSharper disable once CppInconsistentNaming
  class VK_ModelCache
  {
    DataBuffer<VK_Model> _models;

    // which models are shared, by their keys
    SharedIds<ModelKey> _shared;

    // the files, by path and fit, that could not be read, which was said
    // once, so that they are not read again
    std::set<ModelKey> _failed;

    FileSystemContext *_file_system_context = nullptr;
    VK_Device *_device = nullptr;
    std::shared_ptr<Logger> _logger;

    // how many shared models were read from a file or uploaded from a mesh
    std::size_t _loads = 0;

    /// Frees a model and forgets its id.
    void Free(int id);

  public:
    explicit VK_ModelCache(int capacity) : _models(capacity) {}

    void Initialize(FileSystemContext *file_system_context, VK_Device *device, const std::shared_ptr<Logger> &logger);

    /// Hands over the model a render object draws, loading it when it is
    /// not held yet. Returns its id, or -1 when it cannot be loaded or
    /// there is no room, which is logged. A file that could not be read
    /// before is not read again, and returns -1 without a word.
    int Acquire(const RenderInfo &render_info);

    /// Gives a model back. It is freed when nothing holds it any more.
    void Release(int id);

    /// Frees every model, whatever still held one, and forgets the files
    /// that could not be read.
    void CleanUp();

    [[nodiscard]] bool Contains(const int id) const { return _models.Contains(id); }

    /// The model of an id, or nullptr when there is none.
    [[nodiscard]] const VK_Model *Get(const int id) const { return _models.Find(id); }

    /// Draws the model at `id`, which was made from a mesh of its render
    /// object's own, with `mesh` from now on. A shared model is never
    /// changed, which is logged.
    void UpdateMesh(int id, const MeshData &mesh);

    /// How many models are held.
    [[nodiscard]] int Size() const { return _models.Size(); }

    /// How many shared models were read from a file or uploaded from a mesh
    /// since the cache was made.
    [[nodiscard]] std::size_t Loads() const { return _loads; }

    /// How many times a render object took a model that was held already.
    [[nodiscard]] std::size_t Shares() const { return _shared.Shares(); }

    /// How many files, by path and fit, could not be read and are not read
    /// again.
    [[nodiscard]] std::size_t Failures() const { return _failed.size(); }
  };
} // neon

#endif //VK_MODEL_CACHE_HPP
