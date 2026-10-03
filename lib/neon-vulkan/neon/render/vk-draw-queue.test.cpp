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

  TEST(VkDrawQueueTest, DrawsThatCastAndDrawsThatDoNotAreOneBatchOfTheSceneAndTheCastersOneOfTheShadowMap)
  {
    VK_DrawQueue queue;
    VK_Draw casts = Draw(1, 1, 1, 1.0f);
    casts.shadow_pipeline = Pipeline(9);
    VK_Draw does_not = Draw(1, 1, 1, 2.0f);
    does_not.casts_shadow = false;
    queue.Add(casts);
    queue.Add(does_not);
    queue.Settle();

    EXPECT_EQ(queue.Batches().size(), 1u);
    ASSERT_EQ(queue.ShadowBatches().size(), 1u);
    EXPECT_EQ(queue.ShadowBatches()[0].instances, 1u);
    EXPECT_EQ(queue.ShadowBatches()[0].first_instance, 0u);
  }

  TEST(VkDrawQueueTest, CratesOfTwoMaterialsAreTwoBatchesOfTheSceneAndOneOfTheShadowMap)
  {
    // the same model, the same pipeline, two materials: side by side in the
    // buffer, and the pass does not care which material each has
    VK_DrawQueue queue;
    for (int i = 0; i < 4; i++)
    {
      VK_Draw draw = Draw(1, i < 2 ? 1 : 2, 7, static_cast<float>(i));
      draw.shadow_pipeline = Pipeline(9);
      queue.Add(draw);
    }
    queue.Settle();

    EXPECT_EQ(queue.Batches().size(), 2u);
    ASSERT_EQ(queue.ShadowBatches().size(), 1u);
    EXPECT_EQ(queue.ShadowBatches()[0].first_instance, 0u);
    EXPECT_EQ(queue.ShadowBatches()[0].instances, 4u);
    EXPECT_EQ(queue.ShadowBatches()[0].model_id, 7);
  }

  TEST(VkDrawQueueTest, CastersOfOneModelAreOneBatchWhereverTheyStandAmongTheDraws)
  {
    // another model stands between them in the scene's order; the pass
    // has an order of its own
    VK_DrawQueue queue;
    VK_Draw first = Draw(1, 1, 7, 1.0f);
    VK_Draw between = Draw(1, 1, 8, 1.0f);
    VK_Draw second = Draw(1, 2, 7, 1.0f);
    for (VK_Draw *draw : {&first, &between, &second}) { draw->shadow_pipeline = Pipeline(9); }
    queue.Add(first);
    queue.Add(between);
    queue.Add(second);
    queue.Settle();

    // in the scene: model 7 at 0, model 8 at 1, model 7 at 2
    ASSERT_EQ(queue.ShadowBatches().size(), 2u);
    EXPECT_EQ(queue.ShadowBatches()[0].model_id, 7);
    EXPECT_EQ(queue.ShadowBatches()[0].first_instance, 0u);
    EXPECT_EQ(queue.ShadowBatches()[0].instances, 2u);
    EXPECT_EQ(queue.ShadowBatches()[1].model_id, 8);
    EXPECT_EQ(queue.ShadowBatches()[1].first_instance, 2u);
    EXPECT_EQ(queue.ShadowOrder(), (std::vector<uint32_t>{0, 2, 1}));
  }

  TEST(VkDrawQueueTest, ADrawThatCastsTheWholeModelAndOneThatCastsOneMaterialAreNotOneBatch)
  {
    VK_DrawQueue queue;
    VK_Draw whole = Draw(1, 1, 7, 1.0f);
    whole.shadow_pipeline = Pipeline(9);
    whole.shadow_material = -1;
    VK_Draw part = Draw(1, 1, 7, 2.0f);
    part.shadow_pipeline = Pipeline(9);
    part.shadow_material = 2;
    queue.Add(whole);
    queue.Add(part);
    queue.Settle();

    ASSERT_EQ(queue.ShadowBatches().size(), 2u);
    EXPECT_EQ(queue.ShadowBatches()[0].model_material, -1);
    EXPECT_EQ(queue.ShadowBatches()[1].model_material, 2);
  }

  TEST(VkDrawQueueTest, SeeThroughDrawsCastNothing)
  {
    VK_DrawQueue queue;
    VK_Draw glass = Draw(1, 1, 7, 1.0f, true);
    glass.shadow_pipeline = Pipeline(9);
    queue.Add(glass);
    queue.Settle();

    EXPECT_TRUE(queue.ShadowBatches().empty());
  }

  TEST(VkDrawQueueTest, TheMeshesOfTwoMaterialsOfOneModelAreTwoBatches)
  {
    VK_DrawQueue queue;
    VK_Draw first = Draw(1, 1, 1, 1.0f);
    first.model_material = 0;
    VK_Draw second = Draw(1, 1, 1, 1.0f);
    second.model_material = 1;
    queue.Add(second);
    queue.Add(first);
    queue.Add(second);
    queue.Settle();

    ASSERT_EQ(queue.Batches().size(), 2u);
    EXPECT_EQ(queue.Batches()[0].model_material, 0);
    EXPECT_EQ(queue.Batches()[0].instances, 1u);
    EXPECT_EQ(queue.Batches()[1].model_material, 1);
    EXPECT_EQ(queue.Batches()[1].instances, 2u);
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
