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

    // todo uncomment the next two lines for performance optimization later
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

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
    const auto mesh = _mesh_refs[mesh_id];

    mesh.Use();
  }

  void OpenGL_RenderSystem::DestroyMesh(const int mesh_id)
  {
    std::cout << "Cleaning up mesh with id " << mesh_id << std::endl;

    auto mesh = _mesh_refs.Remove(mesh_id);
    mesh.CleanUp();
  }

  int OpenGL_RenderSystem::InitMaterial(
    const std::string &shader_path,
    const std::vector<std::string> &texture_paths,
    const Color &color)
  {
    auto material = OpenGL_Material(shader_path, texture_paths, color);

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
    const auto material = _material_refs[material_id];
    material.Use(model, view, projection);
  }

  void OpenGL_RenderSystem::DestroyMaterial(const int material_id)
  {
    std::cout << "Cleaning up material with id " << material_id << std::endl;

    auto material = _material_refs.Remove(material_id);
    material.CleanUp();
  }

  const RenderResolution& OpenGL_RenderSystem::GetRenderResolution()
  {
    return _render_resolution;
  }

  int OpenGL_RenderSystem::CreateRenderObject(
    const std::string &model_path,
    const std::string &shader_path,
    const std::vector<std::string> &texture_paths,
    const Color &color)
  {
    std::vector<float> vertices(0);
    std::vector<float> normals(0);
    std::vector<float> uvs(0);
    std::vector<unsigned int> indices(0);
    load_obj(model_path, vertices, normals, uvs, indices);

    return CreateRenderObject(vertices, normals, uvs, indices, shader_path, texture_paths, color);
  }

  int OpenGL_RenderSystem::CreateRenderObject(
    const std::vector<float> &vertices,
    const std::vector<float> &normals,
    const std::vector<float> &uvs,
    const std::vector<unsigned int> &indices,
    const std::string &shader_path,
    const std::vector<std::string> &texture_paths,
    const Color &color)
  {
    // todo optimization opportunity
    // find a way to reuse materials and meshes already created
    // the same materials can make use of different materials and vice versa
    auto mesh = OpenGL_Mesh(vertices, normals, uvs, indices);
    auto material = OpenGL_Material(shader_path, texture_paths, color);

    if (!mesh.Initialize())
    {
      std::cerr << "Could not initialize mesh" << std::endl;
      return -1;
    }

    if (!material.Initialize())
    {
      std::cerr << "Could not initialize material" << std::endl;
      return -1;
    }

    const auto mesh_id = _mesh_refs.Add(mesh);
    const auto material_id = _material_refs.Add(material);

    const auto render_object = RenderObject{
      .mesh_id = mesh_id,
      .material_id = material_id
    };
    return _render_object_buffer.Add(render_object);
  }

  void OpenGL_RenderSystem::DrawRenderObject(
    const int render_object_id,
    const Transform &transform,
    const glm::mat4 &view,
    const glm::mat4 &projection)
  {
    const auto [mesh_id, material_id] = _render_object_buffer[render_object_id];
    const auto mesh = _mesh_refs[mesh_id];
    const auto material = _material_refs[material_id];

    const auto model = mesh.GetModelMatrix();
    material.Use(model, view, projection);
    mesh.Use();
  }

  void OpenGL_RenderSystem::DestroyRenderObject(const int render_object_id)
  {
    const auto [mesh_id, material_id] = _render_object_buffer.Remove(render_object_id);
    auto mesh = _mesh_refs.Remove(mesh_id);
    auto material = _material_refs.Remove(material_id);

    mesh.CleanUp();
    material.CleanUp();
  }
} // core
