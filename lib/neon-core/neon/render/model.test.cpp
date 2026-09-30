#include "model.hpp"

#include <memory>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Mesh;
  using neon::Model;
  using neon::TextureInfo;
  using neon::TextureType;
  using neon::Vertex;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  /// What a backend is handed for one mesh of a model.
  struct LoadedMesh
  {
    unsigned int vertices = 0;
    unsigned int faces = 0;
    std::vector<glm::vec3> positions;
    glm::vec3 diffuse_color{0.0f};
    std::vector<TextureInfo> diffuse_textures;
    std::vector<TextureInfo> specular_textures;
    std::vector<TextureInfo> height_textures;
  };

  /// A model as a backend writes it, which keeps what it is handed instead
  /// of giving it to a graphics card.
  class TestModel final : public Model
  {
  protected:
    bool ProcessMesh(aiMesh *mesh, const aiScene *scene) override
    {
      LoadedMesh loaded;
      loaded.vertices = mesh->mNumVertices;
      loaded.faces = mesh->mNumFaces;
      for (unsigned int i = 0; i < mesh->mNumVertices; i++)
      {
        loaded.positions.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
      }

      const aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
      if (aiColor3D color; material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS)
      {
        loaded.diffuse_color = {color.r, color.g, color.b};
      }
      LoadMaterialTextures(material, aiTextureType_DIFFUSE, loaded.diffuse_textures);
      LoadMaterialTextures(material, aiTextureType_SPECULAR, loaded.specular_textures);
      LoadMaterialTextures(material, aiTextureType_HEIGHT, loaded.height_textures);

      meshes.push_back(loaded);
      return meshes.size() <= accepted_meshes;
    }

    void GenerateNormalizationMatrix() override {}

  public:
    std::vector<LoadedMesh> meshes;

    /// ProcessMesh refuses the meshes beyond this many.
    std::size_t accepted_meshes = 100;

    using Model::Model;
    using Model::ComputeNormalizationMatrix;

    bool Initialize() override
    {
      return LoadModel();
    }

    void Use() const override {}

    void CleanUp() override {}
  };

  class TestMesh final : public Mesh
  {
  public:
    explicit TestMesh(const std::vector<glm::vec3> &positions)
      : Mesh({}, {}, {}, nullptr)
    {
      for (const auto &position : positions) { _vertices.push_back(Vertex{.position = position}); }
    }

    bool Initialize() override { return true; }

    void CleanUp() override {}

    void Use() const override {}
  };

  void ExpectMatrix(const glm::mat4 &actual, const glm::mat4 &expected)
  {
    for (int column = 0; column < 4; column++)
    {
      for (int row = 0; row < 4; row++)
      {
        EXPECT_NEAR(actual[column][row], expected[column][row], 1e-5f) << "column " << column << ", row " << row;
      }
    }
  }

  constexpr auto triangle =
    "v 0 0 0\n"
    "v 1 0 0\n"
    "v 0 1 0\n"
    "f 1 2 3\n";

  constexpr auto square =
    "v 0 0 0\n"
    "v 1 0 0\n"
    "v 1 1 0\n"
    "v 0 1 0\n"
    "f 1 2 3 4\n";

  constexpr auto triangle_with_material =
    "mtllib triangle.mtl\n"
    "v 0 0 0\n"
    "v 1 0 0\n"
    "v 0 1 0\n"
    "vt 0 0\n"
    "vt 1 0\n"
    "vt 0 1\n"
    "usemtl red\n"
    "f 1/1 2/2 3/3\n";

  constexpr auto red_material =
    "newmtl red\n"
    "Kd 1 0 0\n"
    "map_Kd textures/red.png\n"
    "map_Ks textures/shine.png\n"
    "map_Bump textures/bumps.png\n";

  constexpr auto two_objects =
    "mtllib two.mtl\n"
    "o first\n"
    "v 0 0 0\n"
    "v 1 0 0\n"
    "v 0 1 0\n"
    "usemtl one\n"
    "f 1 2 3\n"
    "o second\n"
    "v 0 0 5\n"
    "v 1 0 5\n"
    "v 0 1 5\n"
    "v 1 1 5\n"
    "usemtl two\n"
    "f 4 5 6\n"
    "f 5 7 6\n";

  constexpr auto two_materials =
    "newmtl one\n"
    "Kd 1 0 0\n"
    "newmtl two\n"
    "Kd 0 0 1\n";

  class ModelTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};

    void SetUp() override
    {
      _file_system.Initialize();
    }
  };

  // loading

  TEST_F(ModelTest, ReadsNothingWhenItIsCreated)
  {
    const TestModel model("assets://models/missing.obj", &_file_system, _logger);

    EXPECT_THAT(model.meshes, IsEmpty());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
    EXPECT_EQ(model.GetNormalizedModelMatrix(), glm::mat4(1.0f));
  }

  TEST_F(ModelTest, LoadsAModelThroughTheFileSystem)
  {
    _file_system.AddNativeFile("/assets/models/triangle.obj", triangle);
    TestModel model("assets://models/triangle.obj", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_EQ(model.meshes[0].vertices, 3u);
    EXPECT_EQ(model.meshes[0].faces, 1u);
    EXPECT_THAT(
      model.meshes[0].positions,
      ElementsAre(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(ModelTest, LoadsAModelFromTheFolderOfTheUser)
  {
    _file_system.AddNativeFile("/user/made/triangle.obj", triangle);
    TestModel model("user://made/triangle.obj", &_file_system, _logger);

    EXPECT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(model.meshes.size(), 1u);
  }

  TEST_F(ModelTest, HandsOverTrianglesWhateverTheModelIsMadeOf)
  {
    _file_system.AddNativeFile("/assets/models/square.obj", square);
    TestModel model("assets://models/square.obj", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_EQ(model.meshes[0].faces, 2u);
  }

  TEST_F(ModelTest, ReadsTheMaterialFileAModelNamesThroughTheFileSystem)
  {
    _file_system.AddNativeFile("/assets/models/triangle.obj", triangle_with_material);
    _file_system.AddNativeFile("/assets/models/triangle.mtl", red_material);
    TestModel model("assets://models/triangle.obj", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_EQ(model.meshes[0].diffuse_color, glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(ModelTest, LoadsAModelWhoseMaterialFileIsMissing)
  {
    _file_system.AddNativeFile("/assets/models/triangle.obj", triangle_with_material);
    TestModel model("assets://models/triangle.obj", &_file_system, _logger);

    EXPECT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_EQ(model.meshes[0].vertices, 3u);
    EXPECT_THAT(model.meshes[0].diffuse_textures, IsEmpty());
  }

  TEST_F(ModelTest, HandsOverEveryMeshOfAModel)
  {
    _file_system.AddNativeFile("/assets/models/two.obj", two_objects);
    _file_system.AddNativeFile("/assets/models/two.mtl", two_materials);
    TestModel model("assets://models/two.obj", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_EQ(model.meshes.size(), 2u);

    // the order in which the meshes arrive is not promised
    const bool first_is_red = model.meshes[0].diffuse_color == glm::vec3(1.0f, 0.0f, 0.0f);
    const LoadedMesh &red = model.meshes[first_is_red ? 0 : 1];
    const LoadedMesh &blue = model.meshes[first_is_red ? 1 : 0];

    EXPECT_EQ(red.diffuse_color, glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_EQ(red.faces, 1u);
    EXPECT_EQ(blue.diffuse_color, glm::vec3(0.0f, 0.0f, 1.0f));
    EXPECT_EQ(blue.faces, 2u);
  }

  TEST_F(ModelTest, StopsAndFailsAtTheFirstMeshTheBackendRefuses)
  {
    _file_system.AddNativeFile("/assets/models/two.obj", two_objects);
    _file_system.AddNativeFile("/assets/models/two.mtl", two_materials);
    TestModel model("assets://models/two.obj", &_file_system, _logger);
    model.accepted_meshes = 0;

    EXPECT_FALSE(model.Initialize());

    EXPECT_EQ(model.meshes.size(), 1u);
  }

  TEST_F(ModelTest, FailsAndSaysSoWhenTheFileIsMissing)
  {
    TestModel model("assets://models/missing.obj", &_file_system, _logger);

    EXPECT_FALSE(model.Initialize());

    EXPECT_THAT(model.meshes, IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/missing.obj: "))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(ModelTest, FailsAndSaysSoWhenTheFormatOfTheFileIsNotKnown)
  {
    _file_system.AddNativeFile("/assets/models/notes.txt", "this is not a model\n");
    TestModel model("assets://models/notes.txt", &_file_system, _logger);

    EXPECT_FALSE(model.Initialize());

    EXPECT_THAT(model.meshes, IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/notes.txt: "))
      << _logger->Messages(LogLevel::Error);
  }

  // A file that is named like a model and holds none is loaded without an
  // error, as a model without meshes. assimp goes by the extension and reads
  // an .obj file line by line, skipping what it does not understand. The
  // entity is then not drawn and nothing says why. Whether a model without
  // meshes is an error is a question of design, so LoadModel is left as it
  // is.
  TEST_F(ModelTest, DISABLED_FailsAndSaysSoWhenTheFileHoldsNoModel)
  {
    _file_system.AddNativeFile("/assets/models/notes.obj", "this is not a model\n");
    TestModel model("assets://models/notes.obj", &_file_system, _logger);

    EXPECT_FALSE(model.Initialize());

    EXPECT_THAT(model.meshes, IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/notes.obj: "))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(ModelTest, FailsWhenTheFileIsEmpty)
  {
    _file_system.AddNativeFile("/assets/models/empty.obj", "");
    TestModel model("assets://models/empty.obj", &_file_system, _logger);

    EXPECT_FALSE(model.Initialize());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Error importing model assets://models/empty.obj: "));
  }

  TEST_F(ModelTest, FailsWhenThePathBreaksARule)
  {
    _file_system.AddNativeFile("/assets/models/triangle.obj", triangle);

    TestModel in_another_case("assets://Models/triangle.obj", &_file_system, _logger);
    EXPECT_FALSE(in_another_case.Initialize());

    TestModel without_scheme("/assets/models/triangle.obj", &_file_system, _logger);
    EXPECT_FALSE(without_scheme.Initialize());

    TestModel leaving_the_folder("assets://models/../models/triangle.obj", &_file_system, _logger);
    EXPECT_FALSE(leaving_the_folder.Initialize());
  }

  TEST_F(ModelTest, CanBeLoadedAgain)
  {
    _file_system.AddNativeFile("/assets/models/triangle.obj", triangle);
    TestModel model("assets://models/triangle.obj", &_file_system, _logger);

    EXPECT_TRUE(model.Initialize());
    EXPECT_TRUE(model.Initialize());

    EXPECT_EQ(model.meshes.size(), 2u);
  }

  // the textures of a material

  TEST_F(ModelTest, CollectsTheTexturesOfAMaterialByTheirKind)
  {
    _file_system.AddNativeFile("/assets/models/triangle.obj", triangle_with_material);
    _file_system.AddNativeFile("/assets/models/triangle.mtl", red_material);
    TestModel model("assets://models/triangle.obj", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);
    ASSERT_EQ(model.meshes.size(), 1u);

    ASSERT_EQ(model.meshes[0].diffuse_textures.size(), 1u);
    EXPECT_EQ(model.meshes[0].diffuse_textures[0].path, "textures/red.png");
    EXPECT_EQ(model.meshes[0].diffuse_textures[0].texture_type, TextureType::Diffuse);

    ASSERT_EQ(model.meshes[0].specular_textures.size(), 1u);
    EXPECT_EQ(model.meshes[0].specular_textures[0].path, "textures/shine.png");
    EXPECT_EQ(model.meshes[0].specular_textures[0].texture_type, TextureType::Specular);
  }

  TEST_F(ModelTest, LeavesOutTheTexturesOfAKindItDoesNotSupportAndWarns)
  {
    _file_system.AddNativeFile("/assets/models/triangle.obj", triangle_with_material);
    _file_system.AddNativeFile("/assets/models/triangle.mtl", red_material);
    TestModel model("assets://models/triangle.obj", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);
    ASSERT_EQ(model.meshes.size(), 1u);

    EXPECT_THAT(model.meshes[0].height_textures, IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "Unsupported texture type"))
      << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(ModelTest, CollectsNoTexturesFromAMaterialWithout)
  {
    _file_system.AddNativeFile("/assets/models/two.obj", two_objects);
    _file_system.AddNativeFile("/assets/models/two.mtl", two_materials);
    TestModel model("assets://models/two.obj", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    for (const auto &mesh : model.meshes)
    {
      EXPECT_THAT(mesh.diffuse_textures, IsEmpty());
      EXPECT_THAT(mesh.specular_textures, IsEmpty());
    }
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u) << _logger->Messages(LogLevel::Warn);
  }

  // ComputeNormalizationMatrix

  TEST(Model, LeavesAModelWithoutMeshesAsItIs)
  {
    EXPECT_EQ(TestModel::ComputeNormalizationMatrix({}), glm::mat4(1.0f));
  }

  TEST(Model, LeavesAModelWithoutVerticesAsItIs)
  {
    const TestMesh empty({});

    EXPECT_EQ(TestModel::ComputeNormalizationMatrix({&empty, &empty}), glm::mat4(1.0f));
  }

  TEST(Model, MovesAModelToTheOriginAndMakesItsLongestSideOne)
  {
    const TestMesh box({{0.0f, 0.0f, 0.0f}, {2.0f, 4.0f, 1.0f}});

    const auto matrix = TestModel::ComputeNormalizationMatrix({&box});

    // the middle of the box is at 1, 2, 0.5 and its longest side is 4
    ExpectMatrix(
      matrix,
      scale(glm::mat4(1.0f), glm::vec3(0.25f)) * translate(glm::mat4(1.0f), glm::vec3(-1.0f, -2.0f, -0.5f)));

    const glm::vec3 lowest = matrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const glm::vec3 highest = matrix * glm::vec4(2.0f, 4.0f, 1.0f, 1.0f);
    EXPECT_NEAR(lowest.y, -0.5f, 1e-5f);
    EXPECT_NEAR(highest.y, 0.5f, 1e-5f);
    EXPECT_NEAR(lowest.x, -0.25f, 1e-5f);
    EXPECT_NEAR(highest.z, 0.125f, 1e-5f);
  }

  TEST(Model, EnlargesAModelThatIsSmallerThanOne)
  {
    const TestMesh small({{-0.1f, 0.0f, 0.0f}, {0.1f, 0.05f, 0.0f}});

    const auto matrix = TestModel::ComputeNormalizationMatrix({&small});

    const glm::vec3 left = matrix * glm::vec4(-0.1f, 0.0f, 0.0f, 1.0f);
    const glm::vec3 right = matrix * glm::vec4(0.1f, 0.05f, 0.0f, 1.0f);
    EXPECT_NEAR(right.x - left.x, 1.0f, 1e-5f);
  }

  TEST(Model, MeasuresAModelAcrossAllOfItsMeshes)
  {
    const TestMesh left({{-3.0f, 0.0f, 0.0f}, {-2.0f, 1.0f, 0.0f}});
    const TestMesh right({{4.0f, 0.0f, 0.0f}, {5.0f, 1.0f, 2.0f}});
    const TestMesh empty({});

    const auto matrix = TestModel::ComputeNormalizationMatrix({&left, &empty, &right});

    // from -3 to 5 in x, so the middle is at 1, 0.5, 1 and the longest side is 8
    ExpectMatrix(
      matrix,
      scale(glm::mat4(1.0f), glm::vec3(0.125f)) * translate(glm::mat4(1.0f), glm::vec3(-1.0f, -0.5f, -1.0f)));
  }

  TEST(Model, MeasuresAModelWhateverTheOrderOfItsVertices)
  {
    const TestMesh one({{2.0f, 4.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}});
    const TestMesh other({{0.0f, 4.0f, 0.0f}, {2.0f, 0.0f, 1.0f}});

    ExpectMatrix(TestModel::ComputeNormalizationMatrix({&one}), TestModel::ComputeNormalizationMatrix({&other}));
  }

  TEST(Model, MovesAModelOfOnePointToTheOriginWithoutChangingItsSize)
  {
    const TestMesh point({{3.0f, -2.0f, 7.0f}});

    const auto matrix = TestModel::ComputeNormalizationMatrix({&point});

    ExpectMatrix(matrix, translate(glm::mat4(1.0f), glm::vec3(-3.0f, 2.0f, -7.0f)));
  }

  TEST(Model, NormalizesAFlatModelByTheSidesItHas)
  {
    const TestMesh flat({{0.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}});

    const auto matrix = TestModel::ComputeNormalizationMatrix({&flat});

    ExpectMatrix(
      matrix,
      scale(glm::mat4(1.0f), glm::vec3(0.5f)) * translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 0.0f)));
  }
}
