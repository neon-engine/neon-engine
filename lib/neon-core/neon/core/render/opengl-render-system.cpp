#include "opengl-render-system.hpp"

#include "neon/core/common/util.hpp"


namespace core
{
  void OpenGL_RenderSystem::Initialize()
  {
    std::cout << "Initializing OpenGL" << std::endl;

    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(_window_context->GetGlProcAddress())) == 0)
    {
      throw std::runtime_error("Failed to load OpenGL");
    }

    int major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    _logger->Info("OpenGL context version: {}.{}\n", major, minor);

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

    // TODO [issues/6] uncomment the next two lines for performance optimization later
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  const RenderResolution& OpenGL_RenderSystem::GetRenderResolution()
  {
    return _render_resolution;
  }

  int OpenGL_RenderSystem::CreateRenderObject(const RenderInfo &render_info)
  {
    std::vector<float> vertices(0);
    std::vector<float> normals(0);
    std::vector<float> uvs(0);
    std::vector<unsigned int> indices(0);
    load_obj(render_info.model_path, vertices, normals, uvs, indices);

    return CreateRenderObject(
      vertices,
      normals,
      uvs,
      indices,
      render_info.shader_path,
      render_info.texture_paths,
      render_info.color);
  }

  // TODO [issues/7] find out if returning -1 is the best way to handle this, maybe assert calls are better
  int OpenGL_RenderSystem::CreateRenderObject(
    const std::vector<float> &vertices,
    const std::vector<float> &normals,
    const std::vector<float> &uvs,
    const std::vector<unsigned int> &indices,
    const std::string &shader_path,
    const std::vector<std::string> &texture_paths,
    const Color &color)
  {
    // TODO [issues/8] optimization opportunity
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
      mesh.CleanUp();
      return -1;
    }

    const auto mesh_id = _mesh_refs.Add(mesh);
    const auto material_id = _material_refs.Add(material);

    if (mesh_id < 0 || material_id < 0)
    {
      mesh.CleanUp();
      material.CleanUp();
      return -1;
    }

    const auto render_object = RenderObjectRef{
      .mesh_id = mesh_id,
      .material_id = material_id
    };
    const auto render_id = _render_object_buffer.Add(render_object);

    std::cout << "Created render object with id " << render_id << std::endl;
    std::cout << "With mesh id " << mesh_id << std::endl;
    std::cout << "With material id " << material_id << std::endl;
    return render_id;
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

    auto model = mesh.GetModelMatrix();
    const auto position = translate(glm::mat4{1.0f}, transform.position);
    const auto rotation = mat4_cast(transform.rotation.GetQuaternion());
    const auto scale = glm::scale(glm::mat4{1.0f}, transform.scale);
    model = position * rotation * scale * model;
    material.Use(model, view, projection);
    mesh.Use();
  }

  void OpenGL_RenderSystem::DrawRenderObject(
    const int render_object_id,
    const glm::mat4 &to_world,
    const glm::mat4 &view,
    const glm::mat4 &projection)
  {
    const auto [mesh_id, material_id] = _render_object_buffer[render_object_id];
    const auto mesh = _mesh_refs[mesh_id];
    const auto material = _material_refs[material_id];

    auto model = mesh.GetModelMatrix();
    model = to_world * model;
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
