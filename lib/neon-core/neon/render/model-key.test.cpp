#include "model-key.hpp"

#include <memory>

#include <gtest/gtest.h>

namespace
{
  using neon::MeshData;
  using neon::ModelFit;
  using neon::ModelKey;
  using neon::RenderInfo;

  TEST(ModelKeyTest, KnowsAFileByItsPathAndFit)
  {
    const auto key = ModelKey::Of(RenderInfo{.model_path = "assets://models/crate.gltf", .fit = ModelFit::Unit});

    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(key->source, "assets://models/crate.gltf");
    EXPECT_EQ(key->fit, ModelFit::Unit);
  }

  TEST(ModelKeyTest, TellsTheSameFileWithAnotherFitApart)
  {
    const auto fitted = ModelKey::Of(RenderInfo{.model_path = "assets://models/crate.obj", .fit = ModelFit::Unit});
    const auto as_it_is = ModelKey::Of(RenderInfo{.model_path = "assets://models/crate.obj", .fit = ModelFit::None});

    EXPECT_NE(fitted, as_it_is);
  }

  TEST(ModelKeyTest, KnowsAMeshThatWasBuiltByItsKey)
  {
    const auto key = ModelKey::Of(RenderInfo{
      .mesh = std::make_shared<const MeshData>(),
      .mesh_key = "geometry:box",
    });

    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(key->source, "geometry:box");
    EXPECT_EQ(key->fit, ModelFit::None);
  }

  TEST(ModelKeyTest, KnowsTwoMeshesBuiltFromTheSameValuesAsOne)
  {
    const auto first = ModelKey::Of(RenderInfo{.mesh = std::make_shared<const MeshData>(), .mesh_key = "geometry:box"});
    const auto second = ModelKey::Of(RenderInfo{.mesh = std::make_shared<const MeshData>(), .mesh_key = "geometry:box"});

    EXPECT_EQ(first, second);
  }

  TEST(ModelKeyTest, HasNoKeyForAMeshThatIsItsRenderObjectsOwn)
  {
    const auto key = ModelKey::Of(RenderInfo{.mesh = std::make_shared<const MeshData>()});

    EXPECT_FALSE(key.has_value());
  }

  TEST(ModelKeyTest, KnowsAMeshByItsKeyEvenWhenAFileIsNamedToo)
  {
    const auto key = ModelKey::Of(RenderInfo{
      .model_path = "assets://models/crate.obj",
      .mesh = std::make_shared<const MeshData>(),
      .mesh_key = "geometry:box",
    });

    ASSERT_TRUE(key.has_value());
    EXPECT_EQ(key->source, "geometry:box");
  }
} // namespace
