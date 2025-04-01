#ifndef FORWARD_RENDER_PIPELINE_HPP
#define FORWARD_RENDER_PIPELINE_HPP
#include "render-pipeline.hpp"

namespace neon
{
  class Forward_RenderPipeline final : public RenderPipeline
  {
    std::queue<std::tuple<int, Transform> > _render_queue;
    std::vector<LightSource> _light_sources;
    std::size_t _max_light_sources;
    CameraInfo _camera_info;

  public:
    Forward_RenderPipeline(
      RenderContext *render_context,
      std::size_t max_light_sources,
      const std::shared_ptr<Logger> &logger);

    void SetCameraInfo(const CameraInfo &camera_info) override;

    void EnqueueForRendering(
      int render_object_id,
      const Transform &transform) override;

    void Initialize() override;

    void RenderFrame() override;

    void CleanUp() override;

    void EnqueueLightSource(const LightSource &light_source) override;
  };
} // neon

#endif //FORWARD_RENDER_PIPELINE_HPP
