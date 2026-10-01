#ifndef VK_FRAME_STAGES_HPP
#define VK_FRAME_STAGES_HPP

namespace neon
{
  /// Where the drawing of a frame, or of a render target, has got to.
  // ReSharper disable once CppInconsistentNaming
  enum class VK_FrameStage
  {
    /// Nothing was drawn yet.
    Nothing,

    /// Models of a scene are drawn into the scene image, in linear light.
    Scene,

    /// The scene image was resolved into the image that is shown, and what
    /// is drawn in two dimensions goes on top of it.
    Overlay,
  };

  /// What has to happen to get from one stage to another.
  // ReSharper disable once CppInconsistentNaming
  struct VK_StageSteps
  {
    /// The stage cannot be reached any more. What asked for it is left
    /// out.
    bool refuse = false;

    bool begin_scene = false;
    bool end_scene = false;

    /// Draws the scene image into the image that is shown.
    bool resolve = false;

    bool begin_overlay = false;
  };

  /// The order of the stages of a frame: the scene, then what is drawn on
  /// top of it. It is kept apart from the renderer so that it can be
  /// checked without a graphics card.
  // ReSharper disable once CppInconsistentNaming
  struct VK_FrameStages
  {
    /// What it takes to draw a model of the scene. Once the scene was
    /// resolved, it cannot be drawn into again: that would cover what was
    /// drawn on top of it.
    [[nodiscard]] static VK_StageSteps ToScene(VK_FrameStage from);

    /// What it takes to draw on top of the scene, and to finish a frame.
    /// A scene that was drawn is resolved on the way. Where none was, the
    /// image that is shown is cleared instead.
    [[nodiscard]] static VK_StageSteps ToOverlay(VK_FrameStage from);
  };
} // neon

#endif //VK_FRAME_STAGES_HPP
