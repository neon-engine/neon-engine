#ifndef FORWARD_RENDER_PIPELINE_HPP
#define FORWARD_RENDER_PIPELINE_HPP
#include "render-pipeline.hpp"

namespace core
{
  class Forward_RenderPipeline final : public RenderPipeline
  {
    std::queue<std::tuple<int, glm::mat4> > _render_queue;
    CameraInfo _camera_info;

  public:
    explicit Forward_RenderPipeline(RenderContext *render_context)
      : RenderPipeline(render_context) {}

    void SetCameraInfo(const CameraInfo& camera_info);

    void EnqueueForRendering(int render_object_id, const glm::mat4& to_world) override;

    void Initialize() override;

    void RenderFrame() override;

    void CleanUp() override;
  };
} // core

#endif //FORWARD_RENDER_PIPELINE_HPP
