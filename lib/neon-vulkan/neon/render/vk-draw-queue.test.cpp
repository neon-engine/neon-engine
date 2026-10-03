#include "vk-draw-queue.hpp"

#include <gtest/gtest.h>

// The order the draws of a scene end up in is worked out before Vulkan is
// called, so it is checked here, with handles that are numbers.

namespace
{
  using neon::VK_Draw;
  using neon::VK_DrawQueue;

  VkPipeline Pipeline(const int n) { return reinterpret_cast<VkPipeline>(static_cast<std::uintptr_t>(n)); }
  VkDescriptorSet Set(const int n) { return reinterpret_cast<VkDescriptorSet>(static_cast<std::uintptr_t>(n)); }

  VK_Draw Draw(const int pipeline, const int set, const int model, const float distance, const bool see_through = false)
  {
    VK_Draw draw;
    draw.pipeline = Pipeline(pipeline);
    draw.set = Set(set);
    draw.model_id = model;
    draw.distance = distance;
    draw.see_through = see_through;
    draw.data.color = glm::vec4(distance);
    return draw;
  }

  TEST(VkDrawQueueTest, GathersDrawsWithTheSamePipelineMaterialAndModelIntoOneBatch)
  {
    VK_DrawQueue queue;
    for (int i = 0; i < 10; i++) { queue.Add(Draw(1, 1, 7, static_cast<float>(10 - i))); }
    queue.Settle();

    ASSERT_EQ(queue.Batches().size(), 1u);
    EXPECT_EQ(queue.Batches()[0].first_instance, 0u);
    EXPECT_EQ(queue.Batches()[0].instances, 10u);
    EXPECT_EQ(queue.Batches()[0].model_id, 7);
  }

  TEST(VkDrawQueueTest, OrdersByPipelineThenMaterialThenModelThenTheNearestFirst)
  {
    VK_DrawQueue queue;
    queue.Add(Draw(2, 1, 1, 5.0f));
    queue.Add(Draw(1, 2, 1, 5.0f));
    queue.Add(Draw(1, 1, 2, 5.0f));
    queue.Add(Draw(1, 1, 1, 9.0f));
    queue.Add(Draw(1, 1, 1, 3.0f));
    queue.Settle();

    const auto &draws = queue.Draws();
    ASSERT_EQ(draws.size(), 5u);
    EXPECT_EQ(draws[0].pipeline, Pipeline(1));
    EXPECT_EQ(draws[0].set, Set(1));
    EXPECT_EQ(draws[0].model_id, 1);
    EXPECT_FLOAT_EQ(draws[0].distance, 3.0f);
    EXPECT_FLOAT_EQ(draws[1].distance, 9.0f);
    EXPECT_EQ(draws[2].model_id, 2);
    EXPECT_EQ(draws[3].set, Set(2));
    EXPECT_EQ(draws[4].pipeline, Pipeline(2));

    // the two with everything alike are one batch, the others one each
    ASSERT_EQ(queue.Batches().size(), 4u);
    EXPECT_EQ(queue.Batches()[0].instances, 2u);
    EXPECT_EQ(queue.Batches()[0].first_instance, 0u);
    EXPECT_EQ(queue.Batches()[1].first_instance, 2u);
    EXPECT_EQ(queue.Batches()[3].first_instance, 4u);
  }

  TEST(VkDrawQueueTest, TheObjectsDataFollowsItsDraw)
  {
    VK_DrawQueue queue;
    queue.Add(Draw(1, 1, 1, 9.0f));
    queue.Add(Draw(1, 1, 1, 3.0f));
    queue.Settle();

    EXPECT_FLOAT_EQ(queue.Draws()[0].data.color.x, 3.0f);
    EXPECT_FLOAT_EQ(queue.Draws()[1].data.color.x, 9.0f);
  }

  TEST(VkDrawQueueTest, KeepsTheSeeThroughDrawsBehindTheOpaqueOnesInTheirOwnOrder)
  {
    VK_DrawQueue queue;
    queue.Add(Draw(1, 1, 1, 2.0f, true));
    queue.Add(Draw(2, 1, 1, 5.0f));
    queue.Add(Draw(1, 1, 1, 8.0f, true));
    queue.Add(Draw(1, 1, 1, 5.0f));
    queue.Settle();

    EXPECT_EQ(queue.SeeThroughStart(), 2u);
    EXPECT_TRUE(queue.Draws()[2].see_through);
    EXPECT_FLOAT_EQ(queue.Draws()[2].distance, 2.0f);
    EXPECT_FLOAT_EQ(queue.Draws()[3].distance, 8.0f);
    EXPECT_EQ(queue.Batches().size(), 2u);
    EXPECT_EQ(queue.Draws()[0].pipeline, Pipeline(1));
  }

  TEST(VkDrawQueueTest, DrawsThatCastAndDrawsThatDoNotAreNotOneBatch)
  {
    VK_DrawQueue queue;
    VK_Draw casts = Draw(1, 1, 1, 1.0f);
    VK_Draw does_not = Draw(1, 1, 1, 2.0f);
    does_not.casts_shadow = false;
    queue.Add(casts);
    queue.Add(does_not);
    queue.Settle();

    EXPECT_EQ(queue.Batches().size(), 2u);
  }

  TEST(VkDrawQueueTest, IsEmptyAfterItIsCleared)
  {
    VK_DrawQueue queue;
    queue.Add(Draw(1, 1, 1, 1.0f));
    queue.Settle();
    queue.Clear();

    EXPECT_TRUE(queue.Empty());
    EXPECT_TRUE(queue.Batches().empty());
  }
}
