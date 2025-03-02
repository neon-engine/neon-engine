#include "forward-render-pipeline.hpp"

namespace core
{
  Forward_RenderPipeline::Forward_RenderPipeline(
    RenderContext *render_context,
    const std::size_t max_light_sources,
    const std::shared_ptr<Logger> &logger)
    : RenderPipeline(render_context, logger)
  {
    _max_light_sources = max_light_sources;
    _light_sources.reserve(max_light_sources);
  }

  void Forward_RenderPipeline::SetCameraInfo(const CameraInfo &camera_info)
  {
    _camera_info = camera_info;
  }

  void Forward_RenderPipeline::EnqueueForRendering(
    int render_object_id,
    const glm::mat4 &to_world)
  {
    _render_queue.emplace(render_object_id, to_world);
  }

  void Forward_RenderPipeline::Initialize()
  {
    _logger->Info("Initializing forward rendering pipeline");
  }

  void Forward_RenderPipeline::RenderFrame()
  {
    const auto [width, height] = _render_context->GetRenderResolution();
    const auto projection = glm::perspective(
      glm::radians(_camera_info.fov),
      static_cast<float>(width) / static_cast<float>(height),
      _camera_info.near,
      _camera_info.far);
    while (!_render_queue.empty())
    {
      auto [render_object_id, to_world] = _render_queue.front();
      _render_queue.pop();
      _render_context->DrawRenderObject(render_object_id, to_world, _camera_info.view, projection, _light_sources);
    }

    _light_sources.clear();
  }

  void Forward_RenderPipeline::CleanUp()
  {
    _logger->Info("Cleaning up forward rendering pipeline");
  }

  void Forward_RenderPipeline::EnqueueLightSource(const LightSource &light_source)
  {
    if (_light_sources.size() < _max_light_sources)
    {
      _light_sources.push_back(light_source);
    } else
    {
      _logger->Warn(
        "Cannot enqueue light source {} as we have reached the limit of number of supported light sources",
        light_source.id);
    }
  }
} // core
