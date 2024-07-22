#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include <iostream>
#include <queue>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace core
{
  struct RenderResolution
  {
    RenderResolution(const int width, const int height)
      : width(width),
        height(height) {}

    const int width;
    const int height;
  };

  class RenderContext
  {
    int _mesh_index = 0;
    int _material_index = 0;
  protected:
    int _max_supported_meshes = 0;
    int _max_supported_materials = 0;
    std::queue<int> _deleted_mesh_ids;
    std::queue<int> _deleted_material_ids;

    ~RenderContext() = default;

  public:
    virtual int InitMesh(std::string model_path) = 0;

    virtual int InitMesh(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &tex_coordinates,
      const std::vector<unsigned int> &indices) = 0;

    virtual void DrawMesh(int mesh_id) = 0;

    virtual void DestroyMesh(int mesh_id) = 0;

    virtual int InitMaterial(std::string shader_path, std::vector<std::string> texture_paths) = 0;

    virtual void UseMaterial(int mesh_id, const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) = 0;

    virtual void DestroyMaterial(int mesh_id) = 0;

    virtual RenderResolution GetRenderResolution() = 0;

    int GetMeshId()
    {
      if (!_deleted_mesh_ids.empty())
      {
        const auto mesh_id = _deleted_mesh_ids.front();
        _deleted_mesh_ids.pop();
        return mesh_id;
      }
      if (_mesh_index < _max_supported_meshes)
      {
        return _mesh_index++;
      }

      std::cerr << "Could not create mesh id as it would exceed the supported "
        << _max_supported_meshes
        << std::endl;

      return -1;
    }

    int GetMaterialId()
    {
      if (!_deleted_material_ids.empty())
      {
        const auto material_id = _deleted_material_ids.front();
        _deleted_material_ids.pop();
        return material_id;
      }

      if (_material_index < _max_supported_materials)
      {
        return _material_index++;
      }

      std::cerr << "Could not create material id as it would exceed the supported "
        << _max_supported_materials
        << std::endl;

      return -1;
    }
  };
} // core

#endif //RENDER_CONTEXT_HPP
