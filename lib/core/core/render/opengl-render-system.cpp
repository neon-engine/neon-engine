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
