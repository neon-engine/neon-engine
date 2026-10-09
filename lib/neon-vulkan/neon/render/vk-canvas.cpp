#include "vk-canvas.hpp"

#include <array>
#include <neon/common/color-space.hpp>

namespace neon
{
  VK_Canvas::VK_Canvas(const std::string &name, VK_CanvasShared *shared) : _name(name), _shared(shared) {}

  void VK_Canvas::SetImages(const VkCommandBuffer commands, const VkFramebuffer framebuffer, const VkExtent2D extent)
  {
    _commands = commands;
    _framebuffer = framebuffer;
    _extent = extent;
  }

  bool VK_Canvas::PrepareScene()
  {
    if (!_scene.IsReady())
    {
      if (!_scene.Initialize(
        _shared->device, _extent.width, _extent.height, _shared->scene_pass, _shared->depth_format))
      {
        _shared->logger->Error("Could not create the scene image of {}", _name);
        return false;
      }
    }

    if (_scene_set == VK_NULL_HANDLE) { _scene_set = _shared->resolve->Keep(_scene.View()); }
    return _scene_set != VK_NULL_HANDLE;
  }

  void VK_Canvas::CleanUp()
  {
    if (_shared == nullptr) { return; }

    _shared->resolve->Release(_scene_set);
    _scene_set = VK_NULL_HANDLE;
    if (_shared->effects != nullptr) { _shared->effects->Release(_scene_effect_set); }
    _scene_effect_set = VK_NULL_HANDLE;
    _light_images.CleanUp();
    _screen_images.CleanUp();
    _scene.CleanUp();
  }

  void VK_Canvas::Begin(const Color &clear)
  {
    // a render pass is begun by what is drawn first
    _clear = clear;
    _stage = VK_FrameStage::Nothing;
    _see_through.clear();
    _sky.reset();
    _effects.clear();
    _screen_effects.clear();
  }

  void VK_Canvas::SetEffects(const std::vector<std::string> &effects, const std::vector<std::string> &screen_effects)
  {
    _effects = effects;
    _screen_effects = screen_effects;
  }

  std::vector<VkPipeline> VK_Canvas::FindEffects(
    const VK_EffectKind kind,
    const std::vector<std::string> &paths,
    VK_EffectImages &images) const
  {
    std::vector<VkPipeline> pipelines;
    if (paths.empty() || _shared->effects == nullptr) { return pipelines; }

    // one that cannot be read or made is left out, and was said
    for (const std::string &path : paths)
    {
      if (const VkPipeline pipeline = _shared->effects->Find(kind, path); pipeline != VK_NULL_HANDLE)
      {
        pipelines.push_back(pipeline);
      }
    }
    if (pipelines.empty()) { return pipelines; }

    if (!images.IsReady())
    {
      const bool is_light = kind == VK_EffectKind::Light;
      if (!images.Initialize(
            _shared->device,
            _shared->effects,
            is_light ? _shared->resolve : nullptr,
            kind,
            is_light ? VK_SceneImage::kFormat : _shared->screen_format,
            _extent))
      {
        _shared->logger->Error("Could not make the pictures the effects of {} are run between", _name);
        pipelines.clear();
      }
    }
    return pipelines;
  }

  void VK_Canvas::BeginEffectPass(const VK_EffectKind kind, const VkFramebuffer framebuffer) const
  {
    VkRenderPassBeginInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass.renderPass = _shared->effects->PassOf(kind);
    pass.framebuffer = framebuffer;
    pass.renderArea = {{0, 0}, _extent};

    vkCmdBeginRenderPass(_commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
  }

  VkDescriptorSet VK_Canvas::RunLightEffects()
  {
    const std::vector<VkPipeline> pipelines = FindEffects(VK_EffectKind::Light, _effects, _light_images);
    if (pipelines.empty()) { return _scene_set; }

    if (_scene_effect_set == VK_NULL_HANDLE) { _scene_effect_set = _shared->effects->Keep(_scene.View()); }
    if (_scene_effect_set == VK_NULL_HANDLE) { return _scene_set; }

    // the first reads the scene image, and each one after it what the one
    // before it wrote
    VkDescriptorSet read = _scene_effect_set;
    std::size_t written = 0;
    for (std::size_t i = 0; i < pipelines.size(); i++)
    {
      written = i % 2;
      BeginEffectPass(VK_EffectKind::Light, _light_images.Framebuffer(written));
      _shared->effects->Draw(_commands, pipelines[i], read, _extent);
      vkCmdEndRenderPass(_commands);
      read = _light_images.EffectSet(written);
    }
    return _light_images.ResolveSet(written);
  }

  bool VK_Canvas::EnterScene()
  {
    const VK_StageSteps steps = VK_FrameStages::ToScene(_stage);

    if (steps.refuse)
    {
      if (!_shared->warned_about_order)
      {
        _shared->warned_about_order = true;
        _shared->logger->Warn("A model was drawn after what is drawn on top of the scene, and is left out");
      }
      return false;
    }
    if (!steps.begin_scene) { return true; }

    if (!PrepareScene()) { return false; }

    // the color asked for is sRGB, and the scene image holds light
    const Color clear = SrgbToLinear(_clear);

    std::array<VkClearValue, 2> clears{};
    clears[0].color = {{clear.r * clear.a, clear.g * clear.a, clear.b * clear.a, clear.a}};
    clears[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo pass{};
    pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass.renderPass = _shared->scene_pass;
    pass.framebuffer = _scene.Framebuffer();
    pass.renderArea = {{0, 0}, _extent};
    pass.clearValueCount = static_cast<uint32_t>(clears.size());
    pass.pClearValues = clears.data();

    vkCmdBeginRenderPass(_commands, &pass, VK_SUBPASS_CONTENTS_INLINE);

    // Vulkan counts rows from the top, while the projection of the core
    // assumes they are counted from the bottom. A viewport of negative
    // height, starting at the bottom, turns the picture the right way up.
    const VkViewport viewport{
      0.0f,
      static_cast<float>(_extent.height),
      static_cast<float>(_extent.width),
      -static_cast<float>(_extent.height),
      0.0f,
      1.0f};
    const VkRect2D scissor{{0, 0}, _extent};

    vkCmdSetViewport(_commands, 0, 1, &viewport);
    vkCmdSetScissor(_commands, 0, 1, &scissor);

    _stage = VK_FrameStage::Scene;
    return true;
  }

  void VK_Canvas::EnterOverlay()
  {
    const VK_StageSteps steps = VK_FrameStages::ToOverlay(_stage);

    if (steps.end_scene)
    {
      // what is opaque was kept until now, and is drawn in the order that
      // costs the least, then the sky wherever nothing of it is, then what
      // is see-through over both
      if (_shared->draw_opaque) { _shared->draw_opaque(*this); }
      if (_sky.has_value() && _shared->sky != nullptr) { _shared->sky->Draw(_commands, *_sky); }
      _sky.reset();
      DrawSeeThrough();
      vkCmdEndRenderPass(_commands);
    }

    // The effects of the camera, between its scene and what is drawn on
    // top. Those on the light are run first, and the resolve step reads
    // what they made. Those on the colors of the screen follow the
    // resolve: it writes into a picture of theirs, and the last of them
    // writes into the image that is shown, where the resolve would.
    VkDescriptorSet resolved_from = _scene_set;
    std::vector<VkPipeline> screen_effects;
    VkDescriptorSet last_read = VK_NULL_HANDLE;
    if (steps.resolve)
    {
      resolved_from = RunLightEffects();

      screen_effects = FindEffects(VK_EffectKind::Screen, _screen_effects, _screen_images);
      if (!screen_effects.empty())
      {
        BeginEffectPass(VK_EffectKind::Screen, _screen_images.Framebuffer(0));
        _shared->resolve->Draw(_commands, resolved_from, _extent);
        vkCmdEndRenderPass(_commands);

        std::size_t read = 0;
        for (std::size_t i = 0; i + 1 < screen_effects.size(); i++)
        {
          BeginEffectPass(VK_EffectKind::Screen, _screen_images.Framebuffer(1 - read));
          _shared->effects->Draw(_commands, screen_effects[i], _screen_images.EffectSet(read), _extent);
          vkCmdEndRenderPass(_commands);
          read = 1 - read;
        }
        last_read = _screen_images.EffectSet(read);
      }
    }

    if (steps.begin_overlay)
    {
      // where a scene was drawn the resolve covers what is cleared here
      VkClearValue clear_value{};
      clear_value.color = {{_clear.r * _clear.a, _clear.g * _clear.a, _clear.b * _clear.a, _clear.a}};

      VkRenderPassBeginInfo pass{};
      pass.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      pass.renderPass = _shared->frame_pass;
      pass.framebuffer = _framebuffer;
      pass.renderArea = {{0, 0}, _extent};
      pass.clearValueCount = 1;
      pass.pClearValues = &clear_value;

      vkCmdBeginRenderPass(_commands, &pass, VK_SUBPASS_CONTENTS_INLINE);
    }

    if (steps.resolve)
    {
      if (screen_effects.empty()) { _shared->resolve->Draw(_commands, resolved_from, _extent); }
      else { _shared->effects->Draw(_commands, screen_effects.back(), last_read, _extent); }
    }

    _stage = VK_FrameStage::Overlay;
  }

  void VK_Canvas::DrawSeeThrough()
  {
    VK_DrawOrder::BackToFront(_see_through);

    const VK_ModelCache &models = *_shared->models;

    for (const auto &draw : _see_through)
    {
      // a model that was destroyed since is left out
      if (!models.Contains(draw.model_id)) { continue; }

      vkCmdBindPipeline(_commands, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.pipeline);
      vkCmdBindDescriptorSets(
        _commands, VK_PIPELINE_BIND_POINT_GRAPHICS, _shared->pipeline_layout, 0, 1, &draw.set, 1, &draw.scene_offset);

      models[draw.model_id].Draw(_commands, 1, draw.object_index, draw.material);
    }

    _see_through.clear();
  }

  void VK_Canvas::Leave()
  {
    EnterOverlay();
    vkCmdEndRenderPass(_commands);
    _stage = VK_FrameStage::Nothing;
  }
} // neon
