#include "gl-render-system.hpp"

#include "gl-material.hpp"
#include "gl-model.hpp"
#include "neon/common/util.hpp"


namespace neon
{
  void GL_RenderSystem::Initialize()
  {
    _logger->Info("Initializing OpenGL");

    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(_window_context->GetGlProcAddress())) == 0)
    {
      throw std::runtime_error("Failed to load OpenGL");
    }

    int major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    _logger->Info("OpenGL context version: {}.{}", major, minor);

    glEnable(GL_DEPTH_TEST);

    int num_vertex_attributes_supported;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_vertex_attributes_supported);
    _logger->Debug("Maximum number of vertex attributes supported: {}", num_vertex_attributes_supported);

    glViewport(0, 0, _settings_config.width, _settings_config.height);
  }

  void GL_RenderSystem::CleanUp()
  {
    _logger->Info("Cleaning up OpenGL");
  }

  void GL_RenderSystem::PrepareFrame()
  {
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); for wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // TODO [issues/6] uncomment the next two lines for performance optimization later
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  const RenderResolution& GL_RenderSystem::GetRenderResolution()
  {
    return _render_resolution;
  }

  // TODO [issues/7] find out if returning -1 is the best way to handle this, maybe assert calls are better
  int GL_RenderSystem::CreateRenderObject(const RenderInfo &render_info)
  {
    GL_Model model(render_info.model_path, _logger);
    GL_Material material(
      render_info.shader_path,
      render_info.texture_paths,
      render_info.material_info,
      render_info.scale_textures,
      _logger);

    if (!model.Initialize())
    {
      _logger->Error("Could not initialize model {}", render_info.model_path);
      return -1;
    }

    if (!material.Initialize())
    {
      _logger->Error("Could not initialize material with shader {}", render_info.shader_path);
      model.CleanUp();
      return -1;
    }

    const auto model_id = _model_refs.Add(model);
    const auto material_id = _material_refs.Add(material);

    if (model_id < 0 || material_id < 0)
    {
      model.CleanUp();
      material.CleanUp();
      return -1;
    }

    const auto render_id = _render_object_buffer.Add(RenderObjectRef{
      .model_id = model_id,
      .material_id = material_id
    });

    _logger->Debug(
      "Created render object {} from {} with model id {} and material id {}",
      render_id,
      render_info.model_path,
      model_id,
      material_id);

    return render_id;
  }

  void GL_RenderSystem::DrawRenderObject(
    const int render_object_id,
    const Transform &transform,
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<LightSource> &lights)
  {
    const auto [model_id, material_id] = _render_object_buffer[render_object_id];
    const auto model = _model_refs[model_id];
    const auto material = _material_refs[material_id];

    auto normalized_model_matrix = model.GetNormalizedModelMatrix();
    normalized_model_matrix = transform.world_coordinates * normalized_model_matrix;
    material.Use(normalized_model_matrix, view, projection, transform, lights);
    model.Use();
  }

  void GL_RenderSystem::DestroyRenderObject(const int render_object_id)
  {
    const auto [model_id, material_id] = _render_object_buffer.Remove(render_object_id);
    auto normalized_model_matrix = _model_refs.Remove(model_id);
    auto material = _material_refs.Remove(material_id);

    normalized_model_matrix.CleanUp();
    material.CleanUp();
  }
} // neon
