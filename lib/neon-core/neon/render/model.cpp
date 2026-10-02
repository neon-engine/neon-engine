#include "model.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <cstring>
#include <stack>
#include <string_view>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/postprocess.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace
{
  /// Hands assimp a file that has already been read into memory.
  class MemoryStream final : public Assimp::IOStream
  {
    std::vector<unsigned char> _contents;
    size_t _position = 0;

  public:
    explicit MemoryStream(std::vector<unsigned char> contents) : _contents(std::move(contents)) {}

    size_t Read(void *buffer, const size_t size, const size_t count) override
    {
      if (size == 0 || count == 0) { return 0; }

      const size_t available = (_contents.size() - _position) / size;
      const size_t items = std::min(count, available);
      std::memcpy(buffer, _contents.data() + _position, items * size);
      _position += items * size;
      return items;
    }

    // models are only ever read
    size_t Write(const void *, size_t, size_t) override { return 0; }

    aiReturn Seek(const size_t offset, const aiOrigin origin) override
    {
      size_t target = offset;
      if (origin == aiOrigin_CUR) { target = _position + offset; }
      if (origin == aiOrigin_END) { target = _contents.size() + offset; }

      if (target > _contents.size()) { return aiReturn_FAILURE; }

      _position = target;
      return aiReturn_SUCCESS;
    }

    [[nodiscard]] size_t Tell() const override { return _position; }

    [[nodiscard]] size_t FileSize() const override { return _contents.size(); }

    void Flush() override {}
  };

  /// assimp builds the path of a file that a model refers to by putting the
  /// folder of the model in front of it. When the name it starts from already
  /// carries that folder, the result holds the scheme twice, as in
  /// `assets://models/assets://models/cube.mtl`. The last scheme marks where
  /// the path that was meant begins.
  std::string without_repeated_folder(const char *file)
  {
    const std::string_view path(file);

    const size_t marker = path.rfind("://");
    if (marker == std::string_view::npos) { return std::string(path); }

    // the name of the scheme is the run of letters in front of the marker
    size_t start = marker;
    while (start > 0 && std::isalpha(static_cast<unsigned char>(path[start - 1]))) { start--; }

    return std::string(path.substr(start));
  }

  /// Routes every file assimp opens through the engine's file system. That
  /// covers the model itself and anything it refers to, such as the material
  /// file named by an .obj.
  class FileSystemIoHandler final : public Assimp::IOSystem
  {
    neon::FileSystemContext *_file_system_context;

  public:
    explicit FileSystemIoHandler(neon::FileSystemContext *file_system_context)
      : _file_system_context(file_system_context) {}

    bool Exists(const char *file) const override
    {
      return _file_system_context->Exists(without_repeated_folder(file));
    }

    [[nodiscard]] char getOsSeparator() const override { return '/'; }

    Assimp::IOStream *Open(const char *file, const char *mode) override
    {
      // read only
      if (std::string_view(mode).find_first_of("wa+") != std::string_view::npos) { return nullptr; }

      const std::string path = without_repeated_folder(file);

      // assimp probes for optional files, a missing one is not an error
      if (!_file_system_context->Exists(path)) { return nullptr; }

      std::vector<unsigned char> contents;
      if (!_file_system_context->ReadBytes(path, contents)) { return nullptr; }

      return new MemoryStream(std::move(contents));
    }

    void Close(Assimp::IOStream *stream) override { delete stream; }
  };
}

namespace neon
{
  Model::Model(
    const std::string &path,
    FileSystemContext *file_system_context,
    const std::shared_ptr<Logger> &logger,
    const ModelFit fit)
  {
    _path = path;
    _fit = fit;
    _file_system_context = file_system_context;
    _logger = logger;
  }

  bool Model::LoadModel()
  {
    Assimp::Importer import;

    // the importer takes ownership of the handler and deletes it
    import.SetIOHandler(new FileSystemIoHandler(_file_system_context));
    const auto *scene = import.ReadFile(_path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr)
    {
      auto error_message = std::string(import.GetErrorString());
      _logger->Error("Error importing model {}: {}", _path, error_message);
      return false;
    }

    // what a glTF file may carry that the renderer does not draw yet, said
    // once so that nobody wonders why a model stands still
    if (scene->HasAnimations())
    {
      _logger->Info("Model {} has animations, which are not played", _path);
    }
    if (scene->HasCameras() || scene->HasLights())
    {
      _logger->Info("Model {} has cameras or lights, which are left out", _path);
    }

    _drawn_material = -1;
    LoadMaterials(scene);
    return ProcessNode(scene->mRootNode, scene);
  }

  const ModelMaterial *Model::GetDrawnMaterial() const
  {
    if (_drawn_material < 0 || static_cast<size_t>(_drawn_material) >= _materials.size()) { return nullptr; }
    return &_materials[_drawn_material];
  }

  std::string Model::FolderOf(const std::string &model_path)
  {
    const size_t slash = model_path.rfind('/');
    if (slash == std::string::npos) { return ""; }
    return model_path.substr(0, slash + 1);
  }

  void Model::LoadMaterials(const aiScene *scene)
  {
    _materials.clear();
    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
      const aiMaterial *material = scene->mMaterials[i];
      ModelMaterial loaded;
      LoadMaterialTextures(scene, material, aiTextureType_DIFFUSE, loaded.textures);
      LoadMaterialTextures(scene, material, aiTextureType_SPECULAR, loaded.textures);

      // The base colour factor of glTF, which multiplies the texture. The
      // diffuse colour of an .obj is not read: the scenes that exist set the
      // colour themselves, and a Kd of grey would darken them.
      if (aiColor4D color; material->Get(AI_MATKEY_BASE_COLOR, color) == aiReturn_SUCCESS)
      {
        loaded.color = Color{color.r, color.g, color.b, color.a};
      }

      _materials.push_back(loaded);
    }

    // the renderer draws a model with one material, see docs/models.md
    bool several_used = false;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
      several_used = several_used || scene->mMeshes[i]->mMaterialIndex != scene->mMeshes[0]->mMaterialIndex;
    }
    if (several_used)
    {
      _logger->Info("Model {} uses several materials, it is drawn with the first", _path);
    }
  }

  void Model::LoadMaterialTextures(
    const aiScene *scene,
    const aiMaterial *material,
    const aiTextureType &type,
    std::vector<TextureInfo> &textures) const
  {
    for(unsigned int i = 0; i < material->GetTextureCount(type); i++)
    {
      aiString str;
      material->GetTexture(type, i, &str);
      TextureInfo texture;
      switch (type)
      {
        case aiTextureType_DIFFUSE:
        {
          texture.texture_type = TextureType::Diffuse;
          break;
        }
        case aiTextureType_SPECULAR:
        {
          texture.texture_type = TextureType::Specular;
          break;
        }
        default:
        {
          auto type_index = static_cast<int>(type);
          _logger->Warn("Unsupported texture type {} at index {}", type_index, i);
          continue;
        }
      }

      // an image the file carries itself is named by its place in the file
      if (const aiTexture *embedded = scene->GetEmbeddedTexture(str.C_Str()); embedded != nullptr)
      {
        texture.path = str.C_Str();

        // A height of 0 marks an image file, PNG or JPEG, of as many bytes
        // as the width says. Anything else is raw pixels, which no format
        // the loader reads writes into a model.
        if (embedded->mHeight != 0)
        {
          _logger->Warn("Model {} carries texture {} as raw pixels, which is not read", _path, texture.path);
          continue;
        }

        const auto *bytes = reinterpret_cast<const unsigned char *>(embedded->pcData);
        texture.file = std::make_shared<const std::vector<unsigned char>>(bytes, bytes + embedded->mWidth);
        textures.push_back(texture);
        continue;
      }

      // a file next to the model, named from the folder of the model
      std::string name = str.C_Str();
      while (name.rfind("./", 0) == 0) { name.erase(0, 2); }
      texture.path = FolderOf(_path) + name;
      textures.push_back(texture);
    }
  }

  void Model::NoteWhatIsNotShown(const aiMesh *mesh)
  {
    if (mesh->HasVertexColors(0) && !_noted_vertex_colors)
    {
      _noted_vertex_colors = true;
      _logger->Warn("Model {} has vertex colours, which the shaders do not show", _path);
    }
  }

  bool Model::ProcessNode(aiNode *root, const aiScene *scene)
  {
    // each node with where it stands, its own transform under its parents'
    std::stack<std::pair<aiNode *, aiMatrix4x4>> node_stack;
    node_stack.emplace(root, aiMatrix4x4());

    while (!node_stack.empty()) {
      auto [node, parent_transform] = node_stack.top();
      node_stack.pop();

      if (node == nullptr) { continue; }

      const aiMatrix4x4 transform = parent_transform * node->mTransformation;

      // assimp keeps its matrices by rows, glm by columns
      const glm::mat4 placed = glm::transpose(glm::make_mat4(&transform.a1));

      // Process all meshes in this node
      for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        if (_drawn_material < 0) { _drawn_material = static_cast<int>(mesh->mMaterialIndex); }
        NoteWhatIsNotShown(mesh);
        if (!ProcessMesh(mesh, scene, placed)) {
          return false;
        }
      }

      // Push all children onto the stack for later processing
      for (unsigned int i = 0; i < node->mNumChildren; i++) {
        node_stack.emplace(node->mChildren[i], transform);
      }
    }

    return true;
  }

  void Model::ApplyNodeTransform(
    const glm::mat4 &transform, std::vector<Vertex> &vertices, std::vector<unsigned int> &indices)
  {
    // a node that leaves its mesh where it is, which most do, costs nothing
    if (transform == glm::mat4(1.0f)) { return; }

    const auto normal_matrix = glm::mat3(glm::transpose(glm::inverse(transform)));
    for (auto &vertex : vertices)
    {
      vertex.position = glm::vec3(transform * glm::vec4(vertex.position, 1.0f));
      vertex.normal = normal_matrix * vertex.normal;
    }

    // the sign of the volume the transform gives a unit cube, which is below
    // 0 when the mesh is turned inside out
    if (glm::determinant(glm::mat3(transform)) < 0.0f)
    {
      for (size_t i = 0; i + 2 < indices.size(); i += 3) { std::swap(indices[i + 1], indices[i + 2]); }
    }
  }

  glm::mat4 Model::ComputeNormalizationMatrix(const std::vector<const Mesh *> &meshes, const ModelFit fit)
  {
    // the file is taken at its word
    if (fit == ModelFit::None) { return glm::mat4(1.0f); }

    auto lowest = glm::vec3(std::numeric_limits<float>::max());
    auto highest = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto *mesh : meshes)
    {
      for (const auto &vertex : mesh->GetVertices())
      {
        lowest = glm::min(lowest, vertex.position);
        highest = glm::max(highest, vertex.position);
      }
    }

    // nothing was loaded, leave the model as it is
    if (lowest.x > highest.x) { return glm::mat4(1.0f); }

    const glm::vec3 center = (lowest + highest) / 2.0f;
    const glm::vec3 range = highest - lowest;
    const float longest = std::max(std::max(range.x, range.y), range.z);

    if (longest <= 0.0f) { return translate(glm::mat4(1.0f), -center); }

    return scale(glm::mat4(1.0f), glm::vec3(1.0f / longest)) * translate(glm::mat4(1.0f), -center);
  }

  glm::mat4 Model::GetNormalizedModelMatrix() const
  {
    return _model_matrix;
  }
} // neon
