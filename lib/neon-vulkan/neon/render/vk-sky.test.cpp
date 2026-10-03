#include "vk-sky.hpp"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

// What the sky works out before Vulkan is called: the layout its shaders
// are bound to, which has to match sky-box.frag and sky-sphere.frag, what
// tells two skies apart, the order of the faces of a cube, and how a place
// on the screen becomes the direction it is seen in. Drawing one needs a
// device and is not tested here.

namespace
{
  using neon::SkyInfo;
  using neon::SkyType;
  using neon::VK_Sky;
  using neon::VK_SkyValues;

  // depth from 0 to 1, as the renderer hands its projections over
  const glm::mat4 depth_correction(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.5f, 1.0f);

  constexpr float fov = 45.0f;
  constexpr float aspect = 16.0f / 9.0f;

  glm::mat4 Projection()
  {
    return depth_correction * glm::perspective(glm::radians(fov), aspect, 0.1f, 1000.0f);
  }

  /// The direction a place on the screen is seen in, as sky.glsl works it
  /// out.
  glm::vec3 DirectionAt(const VK_SkyValues &values, const float across, const float up)
  {
    const glm::vec4 far = values.to_sky * glm::vec4(across, up, 1.0f, 1.0f);
    return glm::normalize(glm::vec3(far) / far.w);
  }

  void ExpectDirection(const glm::vec3 &direction, const float x, const float y, const float z)
  {
    EXPECT_NEAR(direction.x, x, 1e-3f);
    EXPECT_NEAR(direction.y, y, 1e-3f);
    EXPECT_NEAR(direction.z, z, 1e-3f);
  }

  SkyInfo Box()
  {
    SkyInfo sky;
    sky.right = "right.png";
    sky.left = "left.png";
    sky.top = "top.png";
    sky.bottom = "bottom.png";
    sky.front = "front.png";
    sky.back = "back.png";
    return sky;
  }

  TEST(VkSkyTest, BindsTheImagesApartFromTheSamplerTheyAreReadThrough)
  {
    EXPECT_EQ(VK_Sky::kBindings[0].binding, VK_Sky::kImage_Binding);
    EXPECT_EQ(VK_Sky::kBindings[0].descriptorType, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    EXPECT_EQ(VK_Sky::kBindings[0].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
    EXPECT_EQ(VK_Sky::kBindings[1].binding, VK_Sky::kSampler_Binding);
    EXPECT_EQ(VK_Sky::kBindings[1].descriptorType, VK_DESCRIPTOR_TYPE_SAMPLER);
    EXPECT_EQ(VK_Sky::kBindings[1].stageFlags, VK_SHADER_STAGE_FRAGMENT_BIT);
  }

  TEST(VkSkyTest, BindsTheImagesFirstAndTheSamplerNext)
  {
    EXPECT_EQ(VK_Sky::kImage_Binding, 0u);
    EXPECT_EQ(VK_Sky::kSampler_Binding, 1u);
  }

  TEST(VkSkyTest, TellsTheShaderAMatrixAndFourNumbers)
  {
    // the block Sky of sky.glsl: a mat4 and a vec4
    EXPECT_EQ(sizeof(VK_SkyValues), 80u);
  }

  TEST(VkSkyTest, CanBeCleanedUpWithoutEverBeingInitialized)
  {
    VK_Sky sky;
    sky.CleanUp();
    sky.CleanUp();
  }

  TEST(VkSkyTest, HoldsNoSkyUntilOneIsDrawn)
  {
    const VK_Sky sky;

    EXPECT_EQ(sky.Size(), 0u);
  }

  // what tells two skies apart

  TEST(VkSkyTest, TwoBoxesOfTheSameImagesAreOneSky)
  {
    EXPECT_EQ(VK_Sky::KeyOf(Box()), VK_Sky::KeyOf(Box()));
  }

  TEST(VkSkyTest, ABoxWithAnotherFaceIsAnotherSky)
  {
    SkyInfo other = Box();
    other.top = "clouds.png";

    EXPECT_NE(VK_Sky::KeyOf(Box()), VK_Sky::KeyOf(other));
  }

  TEST(VkSkyTest, ASphereIsAnotherSkyThanABox)
  {
    SkyInfo sphere = Box();
    sphere.type = SkyType::Sphere;
    sphere.texture = "panorama.png";

    EXPECT_NE(VK_Sky::KeyOf(Box()), VK_Sky::KeyOf(sphere));
  }

  TEST(VkSkyTest, ASphereIsToldApartByItsPanoramaAlone)
  {
    SkyInfo day;
    day.type = SkyType::Sphere;
    day.texture = "day.png";
    SkyInfo night = day;
    night.texture = "night.png";
    SkyInfo day_with_faces = day;
    day_with_faces.front = "front.png";

    EXPECT_NE(VK_Sky::KeyOf(day), VK_Sky::KeyOf(night));
    EXPECT_EQ(VK_Sky::KeyOf(day), VK_Sky::KeyOf(day_with_faces));
  }

  TEST(VkSkyTest, ASkyThatIsTurnedOrDimmedReadsTheSameImages)
  {
    SkyInfo turned = Box();
    turned.rotation = 90.0f;
    turned.brightness = 0.25f;

    EXPECT_EQ(VK_Sky::KeyOf(Box()), VK_Sky::KeyOf(turned));
  }

  // the faces of a cube

  TEST(VkSkyTest, PutsTheFacesInTheOrderOfTheLayersOfACube)
  {
    const auto faces = VK_Sky::FacesOf(Box());

    // positive x, negative x, positive y, negative y
    EXPECT_EQ(faces[0], "right.png");
    EXPECT_EQ(faces[1], "left.png");
    EXPECT_EQ(faces[2], "top.png");
    EXPECT_EQ(faces[3], "bottom.png");

    // a cube counts z the other way round: the front of the world, along
    // negative z, is its layer of positive z
    EXPECT_EQ(faces[4], "front.png");
    EXPECT_EQ(faces[5], "back.png");
  }

  // from a place on the screen to a direction

  TEST(VkSkyTest, TheMiddleOfTheScreenIsSeenAlongTheForwardOfTheCamera)
  {
    const auto values = VK_Sky::ValuesOf(SkyInfo{}, glm::mat4(1.0f), Projection());

    ExpectDirection(DirectionAt(values, 0.0f, 0.0f), 0.0f, 0.0f, -1.0f);
  }

  TEST(VkSkyTest, TheEdgesOfTheScreenAreSeenAsFarOutAsTheFieldOfView)
  {
    const auto values = VK_Sky::ValuesOf(SkyInfo{}, glm::mat4(1.0f), Projection());
    const float half = std::tan(glm::radians(fov) / 2.0f);

    // half the field of view upward, and as much as the screen is wider
    // to the right
    const glm::vec3 top = DirectionAt(values, 0.0f, 1.0f);
    EXPECT_NEAR(top.x, 0.0f, 1e-3f);
    EXPECT_NEAR(top.y / -top.z, half, 1e-3f);

    const glm::vec3 right = DirectionAt(values, 1.0f, 0.0f);
    EXPECT_NEAR(right.y, 0.0f, 1e-3f);
    EXPECT_NEAR(right.x / -right.z, half * aspect, 1e-3f);
  }

  TEST(VkSkyTest, ACameraThatTurnsSeesAnotherPartOfTheSky)
  {
    // looking along positive x
    const auto view = glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto values = VK_Sky::ValuesOf(SkyInfo{}, view, Projection());

    ExpectDirection(DirectionAt(values, 0.0f, 0.0f), 1.0f, 0.0f, 0.0f);
  }

  TEST(VkSkyTest, ACameraThatMovesSeesTheSameSky)
  {
    const auto here = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const auto there =
      glm::lookAt(glm::vec3(500.0f, 20.0f, -300.0f), glm::vec3(500.0f, 20.0f, -301.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    const auto seen_here = VK_Sky::ValuesOf(SkyInfo{}, here, Projection());
    const auto seen_there = VK_Sky::ValuesOf(SkyInfo{}, there, Projection());

    const glm::vec3 corner_here = DirectionAt(seen_here, 0.7f, -0.4f);
    const glm::vec3 corner_there = DirectionAt(seen_there, 0.7f, -0.4f);
    ExpectDirection(corner_there, corner_here.x, corner_here.y, corner_here.z);
  }

  TEST(VkSkyTest, ASkyTurnedAQuarterShowsAheadWhatWasToTheRight)
  {
    // turned against the clock seen from above, what was along positive x
    // comes to lie along negative z, where the camera looks
    SkyInfo sky;
    sky.rotation = 90.0f;

    const auto values = VK_Sky::ValuesOf(sky, glm::mat4(1.0f), Projection());

    ExpectDirection(DirectionAt(values, 0.0f, 0.0f), 1.0f, 0.0f, 0.0f);
  }

  TEST(VkSkyTest, HandsOverHowBrightTheSkyIs)
  {
    SkyInfo sky;
    sky.brightness = 0.25f;

    EXPECT_FLOAT_EQ(VK_Sky::ValuesOf(sky, glm::mat4(1.0f), Projection()).settings.x, 0.25f);
    EXPECT_FLOAT_EQ(VK_Sky::ValuesOf(SkyInfo{}, glm::mat4(1.0f), Projection()).settings.x, 1.0f);
  }
}
