#include "forward-render-pipeline.hpp"

#include <algorithm>

namespace neon
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
    // A camera that draws into a texture stands next to the one of the
    // window, and does not take its place.
    if (camera_info.target == RenderTarget::Texture)
    {
      if (camera_info.texture.empty())
      {
        if (!_warned_about_name)
        {
          _warned_about_name = true;
          _logger->Warn("A camera draws into a texture that has no name. What it sees is not drawn");
        }
        return;
      }

      _texture_cameras.push_back(camera_info);
      return;
    }

    _camera_info = camera_info;
  }

  int Forward_RenderPipeline::TargetOf(const CameraInfo &camera)
  {
    if (const auto found = _targets.find(camera.texture); found != _targets.end()) { return found->second; }

    int target = _render_context->FindRenderTarget(camera.texture);

    // what a camera draws into is made at the scale the game sets, with
    // the levels the camera asks for
    if (target < 0)
    {
      const RenderTargetOptions options{.mipmaps = camera.mipmaps, .scales = true};
      target = _render_context->CreateRenderTarget(camera.texture, camera.width, camera.height, options);
    }

    if (target < 0)
    {
      _logger->Error(
        "The texture '{}' of a camera cannot be created at {} by {} pixels, what the camera sees is not drawn",
        camera.texture, camera.width, camera.height);
    }

    // what failed is kept as well, so that it is not tried again
    _targets[camera.texture] = target;
    return target;
  }

  void Forward_RenderPipeline::EnqueueForRendering(
    int render_object_id,
    const Transform &transform)
  {
    _render_queue.emplace(render_object_id, transform);
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
    // what the cameras see that draw into a texture, before the frame that
    // may show it
    if (!_texture_cameras.empty())
    {
      std::vector<std::tuple<int, Transform>> objects;
      for (auto queue = _render_queue; !queue.empty(); queue.pop()) { objects.push_back(queue.front()); }

      for (const auto &camera : _texture_cameras)
      {
        const int target = TargetOf(camera);
        if (target < 0) { continue; }

        int target_width = camera.width;
        int target_height = camera.height;
        (void) _render_context->GetRenderTargetSize(target, target_width, target_height);

        if (!_render_context->BeginRenderTarget(target, camera.clear)) { continue; }

        if (!camera.effects.empty() || !camera.screen_effects.empty())
        {
          _render_context->SetEffects(camera.effects, camera.screen_effects);
        }

        const auto seen = glm::perspective(
          glm::radians(camera.fov),
          static_cast<float>(target_width) / static_cast<float>(std::max(target_height, 1)),
          camera.near,
          camera.far);

        for (const auto &[render_object_id, transform] : objects)
        {
          _render_context->DrawRenderObject(render_object_id, transform, camera.view, seen, _light_sources);
        }

        if (_sky.has_value()) { _render_context->DrawSky(*_sky, camera.view, seen); }

        _render_context->EndRenderTarget();
      }

      _texture_cameras.clear();
    }

    // what is run over the picture of the camera of the window
    if (!_camera_info.effects.empty() || !_camera_info.screen_effects.empty())
    {
      _render_context->SetEffects(_camera_info.effects, _camera_info.screen_effects);
    }

    while (!_render_queue.empty())
    {
      auto [render_object_id, transform] = _render_queue.front();
      _render_queue.pop();
      _render_context->DrawRenderObject(render_object_id, transform, _camera_info.view, projection, _light_sources);
    }

    // behind the models, wherever none was drawn
    if (_sky.has_value()) { _render_context->DrawSky(*_sky, _camera_info.view, projection); }

    _light_sources.clear();
    _sky.reset();
  }

  void Forward_RenderPipeline::CleanUp()
  {
    _logger->Info("Cleaning up forward rendering pipeline");

    for (const auto &[name, target] : _targets)
    {
      if (target >= 0) { _render_context->DestroyRenderTarget(target); }
    }
    _targets.clear();
    _texture_cameras.clear();
    _sky.reset();
  }

  void Forward_RenderPipeline::SetTime(const double seconds, const double delta)
  {
    // the renderer keeps it for the frames it draws
    _render_context->SetShaderTime(seconds, delta);
  }

  void Forward_RenderPipeline::SetSky(const SkyInfo &sky)
  {
    _sky = sky;
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
} // neon
