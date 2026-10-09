#ifndef VK_CANVAS_HPP
#define VK_CANVAS_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <neon/common/color.hpp>
#include <neon/common/data-buffer.hpp>
#include <neon/logging/logger.hpp>

#include "vk-device.hpp"
#include "vk-draw-order.hpp"
#include "vk-effect-images.hpp"
#include "vk-frame-stages.hpp"
#include "vk-model.hpp"
#include "vk-resolve.hpp"
#include "vk-canvas-shared.hpp"
#include "vk-scene-image.hpp"

namespace neon
{
  /// Where the frame, or a render target, is drawn: how far its drawing
  /// has got, what it is cleared to, the scene image its models are lit
  /// in, and the sky and the see-through models kept until its scene is
  /// finished. The
  /// frame and every render target have one, and are drawn the same way
  /// through it.
  ///
  /// The images that are drawn into belong to whoever owns the canvas,
  /// and are told with SetImages(). The scene image belongs to the
  /// canvas, and is made when the first model is drawn into it: a render
  /// target that only shows what is drawn in two dimensions needs none.
  // ReSharper disable once CppInconsistentNaming
  class VK_Canvas
  {
    std::string _name;
    VK_CanvasShared *_shared = nullptr;

    // what is drawn into: the commands, and the image that is shown
    VkCommandBuffer _commands = VK_NULL_HANDLE;
    VkFramebuffer _framebuffer = VK_NULL_HANDLE;
    VkExtent2D _extent{};

    // where the models are lit, and what the resolve step reads it through
    VK_SceneImage _scene;
    VkDescriptorSet _scene_set = VK_NULL_HANDLE;

    // The effects of the camera that draws into the canvas in this frame:
    // those run on the light of its scene, and those run on the colors a
    // screen is given. And the pictures each kind is run between, made when
    // a camera first names an effect of the kind.
    std::vector<std::string> _effects;
    std::vector<std::string> _screen_effects;
    VK_EffectImages _light_images;
    VK_EffectImages _screen_images;

    // what the first effect on the light reads the scene image through
    VkDescriptorSet _scene_effect_set = VK_NULL_HANDLE;

    // what the canvas is cleared to, as an sRGB color, and how far its
    // drawing got in this frame
    Color _clear{0.0f, 0.0f, 0.0f, 1.0f};
    VK_FrameStage _stage = VK_FrameStage::Nothing;
    std::vector<VK_SeeThroughDraw> _see_through;

    // the sky of the scene, drawn between what is opaque and what is
    // see-through
    std::optional<VK_SkyDraw> _sky;

    /// Draws the see-through models that were kept, from the farthest to
    /// the nearest, and forgets them.
    void DrawSeeThrough();

    /// The pipelines of the effects of a list that can be run, in their
    /// order. Empty when the pictures of the kind cannot be made.
    [[nodiscard]] std::vector<VkPipeline> FindEffects(
      VK_EffectKind kind,
      const std::vector<std::string> &paths,
      VK_EffectImages &images) const;

    /// Begins a render pass of the effects that draws into a picture.
    void BeginEffectPass(VK_EffectKind kind, VkFramebuffer framebuffer) const;

    /// Runs the effects on the light of the scene, once the scene is drawn.
    /// Returns what the resolve step reads the outcome through: the scene
    /// image itself when there is no effect to run.
    [[nodiscard]] VkDescriptorSet RunLightEffects();

  public:
    VK_Canvas() = default;

    /// `name` says what the canvas is, as "the frame", when something
    /// goes wrong with it.
    VK_Canvas(const std::string &name, VK_CanvasShared *shared);

    /// Says what is drawn into: the commands that are recorded, the
    /// framebuffer of the image that is shown, and the size of both. Call
    /// it again when they are made again.
    void SetImages(VkCommandBuffer commands, VkFramebuffer framebuffer, VkExtent2D extent);

    /// Makes the scene image, unless there is one, and what the resolve
    /// step reads it through. The first model drawn calls it; the frame
    /// calls it as soon as it has its images, so that it is known from the
    /// start whether there is room.
    bool PrepareScene();

    /// Frees the scene image. The images told with SetImages() stay with
    /// their owner.
    void CleanUp();

    /// Starts a frame: nothing is drawn yet, and what is drawn first clears
    /// the canvas to `clear`, which is an sRGB color.
    void Begin(const Color &clear);

    /// Says which effects the camera that draws into the canvas names, for
    /// this frame: the paths of those run on the light of its scene, and of
    /// those run on the colors a screen is given. Begin() forgets them.
    void SetEffects(const std::vector<std::string> &effects, const std::vector<std::string> &screen_effects);

    /// Gets the canvas to the stage that draws models of the scene.
    /// Returns false when they cannot be drawn any more.
    bool EnterScene();

    /// Gets it to the stage that draws on top of the scene, which resolves
    /// the scene on the way.
    void EnterOverlay();

    /// Finishes what the canvas draws, for the frame.
    void Leave();

    /// Keeps a see-through model until the scene is finished, when it is
    /// drawn over what is opaque.
    void KeepSeeThrough(const VK_SeeThroughDraw &draw) { _see_through.push_back(draw); }

    /// Keeps the sky until the opaque models of the scene are drawn, so
    /// that it is shaded only where none of them is, and is drawn before
    /// what is see-through. A canvas has one sky: the last one kept.
    void KeepSky(const VK_SkyDraw &draw) { _sky = draw; }

    [[nodiscard]] VkCommandBuffer Commands() const { return _commands; }
    [[nodiscard]] VkExtent2D Extent() const { return _extent; }
  };
} // neon

#endif //VK_CANVAS_HPP
