#include "opengl-render-system.hpp"

#include <fstream>
#include <iostream>

#include <stdexcept>
#include <GL/gl3w.h>


namespace core
{
  void OpenGL_RenderSystem::Initialize()
  {
    std::cout << "Initializing OpenGL" << std::endl;
    if (gl3wInit())
    {
      throw std::runtime_error("Failed to initialize gl3w");
    }

    glEnable(GL_DEPTH_TEST);

    int num_vertex_attributes_supported;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_vertex_attributes_supported);
    std::cout << "Maximum number of vertex attributes supported: " << num_vertex_attributes_supported << std::endl;

    glViewport(0, 0, _settings_config.width, _settings_config.height);

    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &_max_texture_units);
  }

  void OpenGL_RenderSystem::CleanUp()
  {
    std::cout << "Cleaning up OpenGL" << std::endl;
  }

  void OpenGL_RenderSystem::PrepareFrame()
  {
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); for wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  RenderContext *OpenGL_RenderSystem::GetContext()
  {
    return this;
  }

  int OpenGL_RenderSystem::InitMesh(
    const std::vector<float> &vertices,
    const std::vector<float> &normals,
    const std::vector<float> &tex_coordinates,
    const std::vector<int> &indices)
  {
    const auto mesh_id = GetMeshId();
    if (mesh_id < 0) { return -1; }
    std::cout << "Initializing mesh: " << mesh_id << std::endl;

    auto mesh = OpenGL_Mesh(vertices, normals, tex_coordinates, indices);

    if (!mesh.Initialize())
    {
      std::cout << "Could not initialize mesh with id " << mesh_id << std::endl;
    }

    _mesh_refs[mesh_id] = mesh;

    return mesh_id;
  }

  void OpenGL_RenderSystem::DrawMesh(const int mesh_id)
  {
    // ReSharper disable once CppUseStructuredBinding
    const auto mesh = _mesh_refs[mesh_id];

    mesh.Use();
  }


  void OpenGL_RenderSystem::DestroyMesh(const int mesh_id)
  {
    std::cout << "Cleaning up mesh with id " << mesh_id << std::endl;

    // ReSharper disable once CppUseStructuredBinding
    auto mesh = _mesh_refs[mesh_id];

    mesh.CleanUp();

    _deleted_mesh_ids.push(mesh_id);
  }

  int OpenGL_RenderSystem::InitMaterial(const std::string shader_path, const std::vector<std::string> texture_paths)
  {
    const auto material_id = GetMaterialId();
    std::cout << "Initializing material with id " << material_id << std::endl;

    auto material = OpenGL_Material(shader_path, texture_paths);

    if (!material.Initialize())
    {
      std::cerr << "Could not initialize material with id " << material_id << std::endl;
      return -1;
    }

    _material_refs[material_id] = material;

    return material_id;
  }

  void OpenGL_RenderSystem::UseMaterial(const int material_id)
  {
    // ReSharper disable once CppUseStructuredBinding
    const auto material = _material_refs[material_id];

    material.Use();
  }

  void OpenGL_RenderSystem::DestroyMaterial(int material_id)
  {
    std::cout << "Cleaning up material with id " << material_id << std::endl;

    auto material = _material_refs[material_id];

    material.CleanUp();

    _deleted_material_ids.push(material_id);
  }

  RenderResolution OpenGL_RenderSystem::GetRenderResolution()
  {
    return RenderResolution{
      _settings_config.width,
      _settings_config.height
    };
  }
} // core
