#ifndef VK_DRAW_QUEUE_HPP
#define VK_DRAW_QUEUE_HPP

#include <cstddef>
#include <vector>

#include "vk-draw.hpp"
#include "vk-draw-batch.hpp"

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

    /// Where the see-through draws start in Draws(), which is after the
    /// opaque ones.
    [[nodiscard]] std::size_t SeeThroughStart() const;

    [[nodiscard]] bool Empty() const { return _draws.empty(); }
    [[nodiscard]] std::size_t Size() const { return _draws.size(); }

    void Clear();
  };
} // neon

#endif //VK_DRAW_QUEUE_HPP
