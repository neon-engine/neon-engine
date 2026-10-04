#ifndef FORWARD_RENDER_PIPELINE_HPP
#define FORWARD_RENDER_PIPELINE_HPP
#include <map>
#include <optional>
#include <queue>
#include <string>
#include <tuple>
#include <vector>

#include "render-pipeline.hpp"

namespace neon
{
  class Forward_RenderPipeline final : public RenderPipeline
  {
    std::queue<std::tuple<int, Transform> > _render_queue;
    std::vector<LightSource> _light_sources;
    std::size_t _max_light_sources;
    CameraInfo _camera_info;

    // the sky of the frame, which every camera of it sees
    std::optional<SkyInfo> _sky;

    // the cameras of the frame that draw into a texture, and the target
    // of each texture by its name. -1 for one that could not be made,
    // which is not tried again
    std::vector<CameraInfo> _texture_cameras;
    std::map<std::string, int> _targets;
    bool _warned_about_name = false;

    /// The target a camera draws into, made when it is first asked for.
    [[nodiscard]] int TargetOf(const CameraInfo &camera);

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

    void SetSky(const SkyInfo &sky) override;

    void SetTime(double seconds, double delta) override;
  };
} // neon

#endif //FORWARD_RENDER_PIPELINE_HPP
