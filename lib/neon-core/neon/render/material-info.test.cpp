#include "material-info.hpp"

#include <optional>

#include <gtest/gtest.h>

namespace
{
  using neon::MaterialInfo;

  TEST(MaterialInfoTest, LeftOutAndWithoutAFactorOfTheFileComesToTheDefaults)
  {
    const MaterialInfo material;

    EXPECT_FLOAT_EQ(material.MetallicWith(std::nullopt), 0.0f);
    EXPECT_FLOAT_EQ(material.RoughnessWith(std::nullopt), 0.5f);
  }

  TEST(MaterialInfoTest, LeftOutTakesTheFactorOfTheFile)
  {
    // a matte dielectric, as the pieces of a kit are
    const MaterialInfo material;

    EXPECT_FLOAT_EQ(material.MetallicWith(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(material.RoughnessWith(1.0f), 1.0f);
  }

  TEST(MaterialInfoTest, WrittenIsMultipliedIntoTheFactorOfTheFile)
  {
    MaterialInfo material;
    material.metallic = 0.5f;
    material.roughness = 0.5f;

    EXPECT_FLOAT_EQ(material.MetallicWith(0.5f), 0.25f);
    EXPECT_FLOAT_EQ(material.RoughnessWith(1.0f), 0.5f);

    // and 0 turns the metal of a file off
    material.metallic = 0.0f;
    EXPECT_FLOAT_EQ(material.MetallicWith(1.0f), 0.0f);
  }

  TEST(MaterialInfoTest, WrittenWithoutAFactorOfTheFileIsTakenAsItIs)
  {
    MaterialInfo material;
    material.metallic = 1.0f;
    material.roughness = 0.2f;

    EXPECT_FLOAT_EQ(material.MetallicWith(std::nullopt), 1.0f);
    EXPECT_FLOAT_EQ(material.RoughnessWith(std::nullopt), 0.2f);
  }
}
