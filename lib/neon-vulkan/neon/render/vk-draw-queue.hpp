#ifndef VK_DRAW_QUEUE_HPP
#define VK_DRAW_QUEUE_HPP

#include <cstddef>
#include <vector>

#include "vk-draw.hpp"
#include "vk-draw-batch.hpp"
#include "vk-shadow-batch.hpp"

namespace neon
{
  /// The draws of one scene, kept until the scene ends, and then put in
  /// the order that costs the least: by pipeline, so that one is bound
  /// when it changes and not for every object; by material within it, for
  /// the descriptor set; by model within that, so that objects drawn with
  /// the same model and material are one call, an instance each; and the
  /// nearest first within that, so that what is hidden is not shaded.
  /// What is see-through is left out of the order and drawn after, from
  /// the farthest to the nearest, see VK_DrawOrder.
  ///
  /// It is kept apart from the graphics card so that it can be checked.
  // ReSharper disable once CppInconsistentNaming
  class VK_DrawQueue
  {
    std::vector<VK_Draw> _draws;
    std::vector<VK_DrawBatch> _batches;
    std::vector<VK_ShadowBatch> _shadow_batches;
    std::vector<uint32_t> _shadow_order;

  public:
    void Add(const VK_Draw &draw) { _draws.push_back(draw); }

    /// Puts the opaque draws in order and gathers those alike into
    /// batches; the see-through draws are moved behind them, in the order
    /// they came. Afterwards the draws, in their order, are what the
    /// buffer of the frame holds, and each batch names its first.
    void Settle();

    /// Every draw, in the order Settle() left them in: the opaque ones
    /// first, as the batches have them, then the see-through ones.
    [[nodiscard]] const std::vector<VK_Draw> &Draws() const { return _draws; }

    /// The opaque draws as batches, in order. Empty before Settle().
    [[nodiscard]] const std::vector<VK_DrawBatch> &Batches() const { return _batches; }

    /// What goes into the shadow map, as batches of their own: the opaque
    /// draws that cast, gathered by the pipeline of the pass, the model, and
    /// its meshes, whatever material each is drawn with in the scene, since
    /// the pass writes depth alone. A batch names its first object by its
    /// place in ShadowOrder(). Empty before Settle().
    [[nodiscard]] const std::vector<VK_ShadowBatch> &ShadowBatches() const { return _shadow_batches; }

    /// The draws that cast, as places in Draws(), in the order the batches
    /// of the pass have them. Their objects are written into the buffer of
    /// the frame once more in this order, after those of the scene, so that
    /// every batch of the pass has its objects side by side.
    [[nodiscard]] const std::vector<uint32_t> &ShadowOrder() const { return _shadow_order; }

    /// Where the see-through draws start in Draws(), which is after the
    /// opaque ones.
    [[nodiscard]] std::size_t SeeThroughStart() const;

    [[nodiscard]] bool Empty() const { return _draws.empty(); }
    [[nodiscard]] std::size_t Size() const { return _draws.size(); }

    void Clear();
  };
} // neon

#endif //VK_DRAW_QUEUE_HPP
