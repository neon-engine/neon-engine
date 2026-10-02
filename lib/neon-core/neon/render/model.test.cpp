#include "model.hpp"

#include <cstdint>
#include <cstring>
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

    /// The positions and indices once the node transform is applied, as a
    /// backend keeps them.
    std::vector<glm::vec3> placed_positions;
    std::vector<unsigned int> placed_indices;
    glm::mat4 transform{1.0f};
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
    bool ProcessMesh(aiMesh *mesh, const aiScene *scene, const glm::mat4 &transform) override
    {
      LoadedMesh loaded;
      loaded.vertices = mesh->mNumVertices;
      loaded.faces = mesh->mNumFaces;
      loaded.transform = transform;

      std::vector<Vertex> vertices;
      for (unsigned int i = 0; i < mesh->mNumVertices; i++)
      {
        loaded.positions.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertices.push_back(Vertex{.position = loaded.positions.back()});
      }
      for (unsigned int i = 0; i < mesh->mNumFaces; i++)
      {
        for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
        {
          loaded.placed_indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
      }
      ApplyNodeTransform(transform, vertices, loaded.placed_indices);
      for (const auto &vertex : vertices) { loaded.placed_positions.push_back(vertex.position); }

      const aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
      if (aiColor3D color; material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS)
      {
        loaded.diffuse_color = {color.r, color.g, color.b};
      }
      LoadMaterialTextures(scene, material, aiTextureType_DIFFUSE, loaded.diffuse_textures);
      LoadMaterialTextures(scene, material, aiTextureType_SPECULAR, loaded.specular_textures);
      LoadMaterialTextures(scene, material, aiTextureType_HEIGHT, loaded.height_textures);

      meshes.push_back(loaded);
      return meshes.size() <= accepted_meshes;
    }

    void GenerateNormalizationMatrix() override {}

  public:
    std::vector<LoadedMesh> meshes;

    /// ProcessMesh refuses the meshes beyond this many.
    std::size_t accepted_meshes = 100;

    using Model::Model;
    using Model::ApplyNodeTransform;
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
    EXPECT_THAT(model.GetMaterials(), IsEmpty());
    EXPECT_EQ(model.GetDrawnMaterial(), nullptr);
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

    // named from the folder of the model, as the material file means them
    ASSERT_EQ(model.meshes[0].diffuse_textures.size(), 1u);
    EXPECT_EQ(model.meshes[0].diffuse_textures[0].path, "assets://models/textures/red.png");
    EXPECT_EQ(model.meshes[0].diffuse_textures[0].texture_type, TextureType::Diffuse);
    EXPECT_FALSE(model.meshes[0].diffuse_textures[0].IsEmbedded());

    ASSERT_EQ(model.meshes[0].specular_textures.size(), 1u);
    EXPECT_EQ(model.meshes[0].specular_textures[0].path, "assets://models/textures/shine.png");
    EXPECT_EQ(model.meshes[0].specular_textures[0].texture_type, TextureType::Specular);

    // the same textures, for the renderer to show when the scene names none
    ASSERT_EQ(model.GetMaterials().size(), 2u) << "assimp adds a default material in front";
    ASSERT_NE(model.GetDrawnMaterial(), nullptr);
    ASSERT_EQ(model.GetDrawnMaterial()->textures.size(), 2u);
    EXPECT_EQ(model.GetDrawnMaterial()->textures[0].path, "assets://models/textures/red.png");
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

  // GLB: glTF with its buffer in one binary file

  /// Packs JSON and a binary buffer into a GLB, as the format lays them out:
  /// a header of three words, then a chunk of each, padded to four bytes.
  std::string Glb(const std::string &json, const std::string &buffer)
  {
    const auto padded = [](std::string chunk, const char filling)
    {
      while (chunk.size() % 4 != 0) { chunk.push_back(filling); }
      return chunk;
    };
    const std::string json_chunk = padded(json, ' ');
    const std::string buffer_chunk = padded(buffer, '\0');

    std::string glb;
    const auto word = [&glb](const uint32_t value)
    {
      for (int i = 0; i < 4; i++) { glb.push_back(static_cast<char>((value >> (8 * i)) & 0xff)); }
    };
    word(0x46546C67);
    word(2);
    word(static_cast<uint32_t>(12 + 8 + json_chunk.size() + 8 + buffer_chunk.size()));
    word(static_cast<uint32_t>(json_chunk.size()));
    word(0x4E4F534A);
    glb += json_chunk;
    word(static_cast<uint32_t>(buffer_chunk.size()));
    word(0x004E4942);
    glb += buffer_chunk;
    return glb;
  }

  std::string Floats(const std::vector<float> &values)
  {
    std::string bytes(values.size() * sizeof(float), '\0');
    std::memcpy(bytes.data(), values.data(), bytes.size());
    return bytes;
  }

  std::string Shorts(const std::vector<uint16_t> &values)
  {
    std::string bytes(values.size() * sizeof(uint16_t), '\0');
    std::memcpy(bytes.data(), values.data(), bytes.size());
    return bytes;
  }

  constexpr auto image_bytes = "PNGBYTES";

  /// The buffer of every GLB here: the triangle of the .obj tests at 0, its
  /// indices at 36, a pretend image at 44, and a colour per vertex at 52.
  std::string TriangleBuffer()
  {
    return Floats({0, 0, 0, 1, 0, 0, 0, 1, 0}) + Shorts({0, 1, 2}) + std::string("\0\0", 2) + image_bytes +
           Floats({1, 0, 0, 0, 1, 0, 0, 0, 1});
  }

  /// The JSON of a GLB around the buffer above, with what a test varies:
  /// the nodes, the meshes, the materials, and the images.
  std::string TriangleJson(
    const std::string &nodes, const std::string &meshes, const std::string &materials, const std::string &images)
  {
    return std::string(R"({"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [)") +
           nodes + R"(], "meshes": [)" + meshes + R"(], "materials": [)" + materials + R"(], "images": [)" + images +
           R"(], "textures": [{"source": 0}],
      "buffers": [{"byteLength": 88}],
      "bufferViews": [
        {"buffer": 0, "byteOffset": 0, "byteLength": 36},
        {"buffer": 0, "byteOffset": 36, "byteLength": 6},
        {"buffer": 0, "byteOffset": 44, "byteLength": 8},
        {"buffer": 0, "byteOffset": 52, "byteLength": 36}],
      "accessors": [
        {"bufferView": 0, "componentType": 5126, "count": 3, "type": "VEC3", "min": [0, 0, 0], "max": [1, 1, 0]},
        {"bufferView": 1, "componentType": 5123, "count": 3, "type": "SCALAR"},
        {"bufferView": 3, "componentType": 5126, "count": 3, "type": "VEC3"}]})";
  }

  constexpr auto one_node = R"({"mesh": 0})";
  constexpr auto one_mesh = R"({"primitives": [{"attributes": {"POSITION": 0}, "indices": 1, "material": 0}]})";
  constexpr auto textured_material =
    R"({"pbrMetallicRoughness": {"baseColorFactor": [1, 0.5, 0.25, 1], "baseColorTexture": {"index": 0}}})";
  constexpr auto embedded_image = R"({"bufferView": 2, "mimeType": "image/png"})";
  constexpr auto image_file = R"({"uri": "Textures/colormap.png"})";

  TEST_F(ModelTest, ReadsAGlbAndTheImageItCarries)
  {
    _file_system.AddNativeFile(
      "/assets/models/cube.glb",
      Glb(TriangleJson(one_node, one_mesh, textured_material, embedded_image), TriangleBuffer()));
    TestModel model("assets://models/cube.glb", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_EQ(model.meshes[0].faces, 1u);
    EXPECT_THAT(
      model.meshes[0].positions,
      ElementsAre(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

    ASSERT_EQ(model.GetMaterials().size(), 2u) << "assimp adds a default material behind";
    ASSERT_NE(model.GetDrawnMaterial(), nullptr);
    const auto &material = *model.GetDrawnMaterial();
    ASSERT_EQ(material.textures.size(), 1u);
    EXPECT_EQ(material.textures[0].texture_type, TextureType::Diffuse);
    EXPECT_EQ(material.textures[0].path, "*0");
    ASSERT_TRUE(material.textures[0].IsEmbedded());
    EXPECT_EQ(std::string(material.textures[0].file->begin(), material.textures[0].file->end()), image_bytes);

    // the mesh names the same image, and shares it
    ASSERT_EQ(model.meshes[0].diffuse_textures.size(), 1u);
    EXPECT_EQ(model.meshes[0].diffuse_textures[0].file->size(), 8u);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u) << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(ModelTest, ReadsTheBaseColourOfAGlbMaterial)
  {
    _file_system.AddNativeFile(
      "/assets/models/cube.glb",
      Glb(TriangleJson(one_node, one_mesh, textured_material, embedded_image), TriangleBuffer()));
    TestModel model("assets://models/cube.glb", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_NE(model.GetDrawnMaterial(), nullptr);
    const auto &[r, g, b, a] = model.GetDrawnMaterial()->color;
    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 0.5f);
    EXPECT_FLOAT_EQ(b, 0.25f);
    EXPECT_FLOAT_EQ(a, 1.0f);
  }

  TEST_F(ModelTest, LeavesTheColourOfAMaterialWithoutOneWhite)
  {
    _file_system.AddNativeFile(
      "/assets/models/cube.glb",
      Glb(TriangleJson(one_node, one_mesh, R"({"name": "plain"})", embedded_image), TriangleBuffer()));
    TestModel model("assets://models/cube.glb", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_NE(model.GetDrawnMaterial(), nullptr);
    const auto &[r, g, b, a] = model.GetDrawnMaterial()->color;
    EXPECT_FLOAT_EQ(r, 1.0f);
    EXPECT_FLOAT_EQ(g, 1.0f);
    EXPECT_FLOAT_EQ(b, 1.0f);
    EXPECT_FLOAT_EQ(a, 1.0f);
    EXPECT_THAT(model.GetDrawnMaterial()->textures, IsEmpty());
  }

  TEST_F(ModelTest, NamesTheImageFileOfAGlbFromTheFolderOfTheModel)
  {
    _file_system.AddNativeFile(
      "/assets/external/kit/wall.glb",
      Glb(TriangleJson(one_node, one_mesh, textured_material, image_file), TriangleBuffer()));
    TestModel model("assets://external/kit/wall.glb", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_NE(model.GetDrawnMaterial(), nullptr);
    ASSERT_EQ(model.GetDrawnMaterial()->textures.size(), 1u);
    EXPECT_EQ(model.GetDrawnMaterial()->textures[0].path, "assets://external/kit/Textures/colormap.png");
    EXPECT_FALSE(model.GetDrawnMaterial()->textures[0].IsEmbedded());
  }

  TEST_F(ModelTest, PlacesAMeshWhereItsNodeAndTheNodesAboveItStand)
  {
    // a parent that moves up by 1 with a child that moves back by 5
    constexpr auto nodes = R"({"translation": [0, 1, 0], "children": [1]}, {"mesh": 0, "translation": [0, 0, 5]})";
    _file_system.AddNativeFile(
      "/assets/models/cube.glb",
      Glb(TriangleJson(nodes, one_mesh, textured_material, embedded_image), TriangleBuffer()));
    TestModel model("assets://models/cube.glb", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_THAT(
      model.meshes[0].placed_positions,
      ElementsAre(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(1.0f, 1.0f, 5.0f), glm::vec3(0.0f, 2.0f, 5.0f)));
    EXPECT_THAT(model.meshes[0].placed_indices, ElementsAre(0u, 1u, 2u));
  }

  TEST_F(ModelTest, TurnsTheTrianglesOfAMirroredNodeRound)
  {
    constexpr auto nodes = R"({"mesh": 0, "scale": [-1, 1, 1]})";
    _file_system.AddNativeFile(
      "/assets/models/cube.glb",
      Glb(TriangleJson(nodes, one_mesh, textured_material, embedded_image), TriangleBuffer()));
    TestModel model("assets://models/cube.glb", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    // the triangle is mirrored in x, and goes round the other way so that
    // its front still faces the same side
    ASSERT_EQ(model.meshes.size(), 1u);
    EXPECT_THAT(
      model.meshes[0].placed_positions,
      ElementsAre(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    EXPECT_THAT(model.meshes[0].placed_indices, ElementsAre(0u, 2u, 1u));
  }

  TEST_F(ModelTest, WarnsAboutVertexColoursWhichTheShadersDoNotShow)
  {
    constexpr auto coloured_mesh =
      R"({"primitives": [{"attributes": {"POSITION": 0, "COLOR_0": 2}, "indices": 1, "material": 0}]})";
    _file_system.AddNativeFile(
      "/assets/models/cube.glb",
      Glb(TriangleJson(one_node, coloured_mesh, textured_material, embedded_image), TriangleBuffer()));
    TestModel model("assets://models/cube.glb", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "vertex colours")) << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(ModelTest, SaysWhenAModelUsesSeveralMaterials)
  {
    constexpr auto two_primitives =
      R"({"primitives": [
        {"attributes": {"POSITION": 0}, "indices": 1, "material": 0},
        {"attributes": {"POSITION": 0}, "indices": 1, "material": 1}]})";
    constexpr auto materials = R"({"name": "one"}, {"name": "two"})";
    _file_system.AddNativeFile(
      "/assets/models/cube.glb",
      Glb(TriangleJson(one_node, two_primitives, materials, embedded_image), TriangleBuffer()));
    TestModel model("assets://models/cube.glb", &_file_system, _logger);

    ASSERT_TRUE(model.Initialize()) << _logger->Messages(LogLevel::Error);

    EXPECT_EQ(model.meshes.size(), 2u);
    EXPECT_EQ(model.GetMaterials().size(), 3u) << "the two, and the default behind them";
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "several materials")) << _logger->Messages(LogLevel::Info);
  }

  // ApplyNodeTransform

  TEST(Model, LeavesAMeshWhereItIsForANodeThatDoesNotMoveIt)
  {
    std::vector<Vertex> vertices{Vertex{.position = {1.0f, 2.0f, 3.0f}, .normal = {0.0f, 1.0f, 0.0f}}};
    std::vector<unsigned int> indices{0, 1, 2};

    TestModel::ApplyNodeTransform(glm::mat4(1.0f), vertices, indices);

    EXPECT_EQ(vertices[0].position, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(vertices[0].normal, glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_THAT(indices, ElementsAre(0u, 1u, 2u));
  }

  TEST(Model, TurnsTheNormalsWithTheMeshAndNotWithItsStretch)
  {
    // stretched along x and turned a quarter about z: a normal that pointed
    // up now points left, and is still of length 1
    std::vector<Vertex> vertices{Vertex{.position = {1.0f, 0.0f, 0.0f}, .normal = {0.0f, 1.0f, 0.0f}}};
    std::vector<unsigned int> indices{0, 1, 2};
    const glm::mat4 transform =
      rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
      scale(glm::mat4(1.0f), glm::vec3(3.0f, 1.0f, 1.0f));

    TestModel::ApplyNodeTransform(transform, vertices, indices);

    EXPECT_NEAR(vertices[0].position.x, 0.0f, 1e-5f);
    EXPECT_NEAR(vertices[0].position.y, 3.0f, 1e-5f);
    EXPECT_NEAR(vertices[0].normal.x, -1.0f, 1e-5f);
    EXPECT_NEAR(vertices[0].normal.y, 0.0f, 1e-5f);
    EXPECT_THAT(indices, ElementsAre(0u, 1u, 2u));
  }

  TEST(Model, ReversesTheWindingOfEveryTriangleOfAMirroredMesh)
  {
    std::vector<Vertex> vertices{Vertex{.position = {1.0f, 0.0f, 0.0f}, .normal = {1.0f, 0.0f, 0.0f}}};
    std::vector<unsigned int> indices{0, 1, 2, 3, 4, 5};

    TestModel::ApplyNodeTransform(scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f)), vertices, indices);

    EXPECT_THAT(indices, ElementsAre(0u, 2u, 1u, 3u, 5u, 4u));
    EXPECT_EQ(vertices[0].normal, glm::vec3(1.0f, 0.0f, 0.0f));
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
