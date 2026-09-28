#include "model.hpp"

#include <algorithm>
#include <cstring>
#include <stack>
#include <string_view>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/postprocess.h>

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
      return _file_system_context->Exists(file);
    }

    [[nodiscard]] char getOsSeparator() const override { return '/'; }

    Assimp::IOStream *Open(const char *file, const char *mode) override
    {
      // read only
      if (std::string_view(mode).find_first_of("wa+") != std::string_view::npos) { return nullptr; }

      // assimp probes for optional files, a missing one is not an error
      if (!_file_system_context->Exists(file)) { return nullptr; }

      std::vector<unsigned char> contents;
      if (!_file_system_context->ReadBytes(file, contents)) { return nullptr; }

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
    const std::shared_ptr<Logger> &logger)
  {
    _path = path;
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

    return ProcessNode(scene->mRootNode, scene);
  }

  void Model::LoadMaterialTextures(
    const aiMaterial *material,
    const aiTextureType &type,
    std::vector<TextureInfo> textures) const
  {
    for(unsigned int i = 0; i < material->GetTextureCount(type); i++)
    {
      aiString str;
      material->GetTexture(type, i, &str);
      TextureInfo texture;
      texture.path = str.C_Str();
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
      textures.push_back(texture);
    }
  }

  bool Model::ProcessNode(aiNode *root, const aiScene *scene)
  {
    std::stack<aiNode *> node_stack;
    node_stack.push(root);

    while (!node_stack.empty()) {
      const aiNode *node = node_stack.top();
      node_stack.pop();

      if (node == nullptr) { continue; }

      // Process all meshes in this node
      for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        if (aiMesh *mesh = scene->mMeshes[node->mMeshes[i]]; !ProcessMesh(mesh, scene)) {
          return false;
        }
      }

      // Push all children onto the stack for later processing
      for (unsigned int i = 0; i < node->mNumChildren; i++) {
        node_stack.push(node->mChildren[i]);
      }
    }

    return true;
  }

  glm::mat4 Model::GetNormalizedModelMatrix() const
  {
    return _model_matrix;
  }
} // neon
