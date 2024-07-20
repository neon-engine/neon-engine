#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include <string>
#include <vector>

namespace core
{
  class RenderContext
  {
  protected:
    int _geometry_index = 0;
    int _material_index = 0;

    ~RenderContext() = default;

  public:
    virtual int InitMesh(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &tex_coordinates,
      const std::vector<int> &indices) = 0;

    virtual void DrawMesh(int geometry_id) = 0;

    virtual void DestroyMesh(int geometry_id) = 0;

    virtual int InitMaterial(std::string shader_path, std::string texture_paths[]) = 0;

    virtual void UseMaterial(int material_id) = 0;

    virtual void DestroyMaterial(int material_id) = 0;
  };
} // core

#endif //RENDER_CONTEXT_HPP
