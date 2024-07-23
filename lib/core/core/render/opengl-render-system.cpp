#include "opengl-render-system.hpp"

#include <fstream>
#include <iostream>

#include <stdexcept>

#include "util.hpp"


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

  int OpenGL_RenderSystem::InitMesh(const std::string model_path, glm::mat4 &normalized_matrix)
  {
    if (const auto extension = get_file_extension(model_path); extension == "obj")
    {
      std::vector<float> vertices(0);
      std::vector<float> normals(0);
      std::vector<float> uvs(0);
      std::vector<unsigned int> indices(0);
      load_obj(model_path, vertices, normals, uvs, indices);

      std::cout << "Loaded mesh data from " << model_path << std::endl;
      return InitMesh(vertices, normals, uvs, indices, normalized_matrix);
    } else
    {
      std::cerr << "Unsupported model format: " << extension << std::endl;
      return -1;
    }
  }

  int OpenGL_RenderSystem::InitMesh(
    const std::vector<float> &vertices,
    const std::vector<float> &normals,
    const std::vector<float> &tex_coordinates,
    const std::vector<unsigned int> &indices,
    glm::mat4 &normalized_matrix)
  {
    auto mesh = OpenGL_Mesh(vertices, normals, tex_coordinates, indices);

    const auto mesh_id = _mesh_refs.Add(mesh);

    if (mesh_id < 0) { return -1; }

    std::cout << "Initializing mesh: " << mesh_id << std::endl;

    if (!mesh.Initialize())
    {
      std::cout << "Could not initialize mesh with id " << mesh_id << std::endl;
      return -1;
    }

    normalized_matrix = mesh.GetModelMatrix();

    return mesh_id;
  }

  void OpenGL_RenderSystem::DrawMesh(const int mesh_id)
  {
    OpenGL_Mesh mesh;
    _mesh_refs.Get(mesh_id, mesh);

    mesh.Use();
  }

  void OpenGL_RenderSystem::DestroyMesh(const int mesh_id)
  {
    std::cout << "Cleaning up mesh with id " << mesh_id << std::endl;

    if (OpenGL_Mesh mesh; _mesh_refs.Remove(mesh_id, mesh))
    {
      mesh.CleanUp();
    }
  }

  int OpenGL_RenderSystem::InitMaterial(const std::string shader_path, const std::vector<std::string> texture_paths)
  {
    auto material = OpenGL_Material(shader_path, texture_paths);

    const auto material_id = _material_refs.Add(material);

    std::cout << "Initializing material with id " << material_id << std::endl;

    if (!material.Initialize())
    {
      std::cerr << "Could not initialize material with id " << material_id << std::endl;
      return -1;
    }

    return material_id;
  }

  void OpenGL_RenderSystem::UseMaterial(const int material_id,
                                        const glm::mat4 &model,
                                        const glm::mat4 &view,
                                        const glm::mat4 &projection)
  {
    // ReSharper disable once CppUseStructuredBinding
    OpenGL_Material material;
    _material_refs.Get(material_id, material);
    material.Use(model, view, projection);
  }

  void OpenGL_RenderSystem::DestroyMaterial(const int material_id)
  {
    std::cout << "Cleaning up material with id " << material_id << std::endl;

    if (OpenGL_Material material; _material_refs.Remove(material_id, material))
    {
      material.CleanUp();
    }
  }

  RenderResolution OpenGL_RenderSystem::GetRenderResolution()
  {
    return RenderResolution{
      _settings_config.width,
      _settings_config.height
    };
  }

  int OpenGL_RenderSystem::CreateRenderObject(std::string model_path,
    std::string shader_path,
    std::vector<std::string> texture_paths)
  {
    return -1;
  }

  int OpenGL_RenderSystem::CreateRenderObject(const std::vector<float> &vertices,
    const std::vector<float> &normals,
    const std::vector<float> &tex_coordinates,
    const std::vector<unsigned int> &indices,
    std::string shader_path,
    std::vector<std::string> texture_paths)
  {
    return -1;
  }

  void OpenGL_RenderSystem::UseRenderObject() {}
  void OpenGL_RenderSystem::DestroyRenderObject() {}
} // core
