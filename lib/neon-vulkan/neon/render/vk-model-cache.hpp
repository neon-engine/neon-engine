#ifndef VK_MODEL_CACHE_HPP
#define VK_MODEL_CACHE_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <neon/common/data-buffer.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/render-info.hpp>

#include "vk-device.hpp"
#include "vk-model.hpp"

namespace neon
{
  /// The models the render objects draw, held once each.
  ///
  /// A model from a file is held once for every path and fit, however many
  /// render objects draw it: the first one loads it, the others take a
  /// reference, and it is freed when the last one releases it. A mesh that
  /// was built at run time belongs to its render object alone, and is
  /// neither looked up nor shared.
  ///
  /// A model is known by an id that stays the same while it is held, which
  /// a render object keeps as its model id.
  // ReSharper disable once CppInconsistentNaming
  class VK_ModelCache
  {
    /// What is known about a model from a file: where it is held, and how
    /// many render objects hold it.
    struct Shared
    {
      int id = -1;
      int count = 0;
    };

    using Key = std::pair<std::string, ModelFit>;

    DataBuffer<VK_Model> _models;
    std::map<Key, Shared> _shared;

    // the key of every shared model by its id, for releasing
    std::map<int, Key> _keys;

    FileSystemContext *_file_system_context = nullptr;
    VK_Device *_device = nullptr;
    std::shared_ptr<Logger> _logger;

    // how many models were read from a file, and how many times one that
    // was read already was taken instead
    std::size_t _loads = 0;
    std::size_t _shares = 0;

    /// Frees a model and forgets its id.
    void Free(int id);

  public:
    explicit VK_ModelCache(int capacity) : _models(capacity) {}

    void Initialize(FileSystemContext *file_system_context, VK_Device *device, const std::shared_ptr<Logger> &logger);

    /// Hands over the model a render object draws, loading it when it is
    /// not held yet. Returns its id, or -1 when it cannot be loaded or
    /// there is no room, which is logged.
    int Acquire(const RenderInfo &render_info);

    /// Gives a model back. It is freed when nothing holds it any more.
    void Release(int id);

    /// Frees every model, whatever still held one.
    void CleanUp();

    [[nodiscard]] bool Contains(const int id) const { return _models.Contains(id); }

    [[nodiscard]] const VK_Model &operator[](const int id) const { return _models[id]; }

    /// How many models are held.
    [[nodiscard]] int Size() const { return _models.Size(); }

    /// How many models were read from a file since the cache was made.
    [[nodiscard]] std::size_t Loads() const { return _loads; }

    /// How many times a render object took a model that was read already.
    [[nodiscard]] std::size_t Shares() const { return _shares; }
  };
} // neon

#endif //VK_MODEL_CACHE_HPP
