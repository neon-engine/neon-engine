#include "vk-frame-stages.hpp"

namespace neon
{
  VK_StageSteps VK_FrameStages::ToScene(const VK_FrameStage from)
  {
    VK_StageSteps steps;

    switch (from)
    {
      case VK_FrameStage::Nothing: steps.begin_scene = true; break;
      case VK_FrameStage::Scene: break;
      case VK_FrameStage::Overlay: steps.refuse = true; break;
    }
    return steps;
  }

  VK_StageSteps VK_FrameStages::ToOverlay(const VK_FrameStage from)
  {
    VK_StageSteps steps;

    switch (from)
    {
      case VK_FrameStage::Nothing:
        steps.begin_overlay = true;
        break;
      case VK_FrameStage::Scene:
        steps.end_scene = true;
        steps.begin_overlay = true;
        steps.resolve = true;
        break;
      case VK_FrameStage::Overlay:
        break;
    }
    return steps;
  }
} // neon
