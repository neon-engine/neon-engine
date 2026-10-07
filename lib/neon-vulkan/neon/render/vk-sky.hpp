#ifndef VK_SKY_HPP
#define VK_SKY_HPP

#include <array>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <glm/glm.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/sky-info.hpp>

#include "vk-device.hpp"
#include "vk-samplers.hpp"
#include "vk-shader.hpp"
#include "vk-sky-draw.hpp"
#include "vk-texture.hpp"

namespace neon
{
  /// The sky: what a camera sees behind every model, in every direction
  /// and endlessly far away. A triangle that covers the scene image is
  /// drawn at the far end of the depth, after the opaque models, so that
  /// only the pixels no model covered are shaded, and before the
  /// see-through ones, which blend over it. Each pixel is turned into the
  /// direction it is seen in, and the images of the sky are read by that
  /// direction: the six faces of a cube, or a panorama around a sphere.
  ///
  /// The images are sRGB colours read as linear light, like the first
  /// texture of a material, and are written into the scene image as they
  /// are: no light of the scene falls on a sky.
  ///
  /// The images of a sky are held from the frame that first draws it
  /// until a frame goes by that does not, so a scene that is left gives
  /// its sky back.
  // ReSharper disable once CppInconsistentNaming
  class VK_Sky
  {
    /// The images of a sky that was drawn, and what its shader reads them
    /// through.
    struct Held
    {
      VK_Texture texture;
      VkDescriptorSet set = VK_NULL_HANDLE;

      // whether it was drawn since ReleaseUnused() looked last
      bool is_drawn = false;
    };

    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    const VK_Samplers *_samplers = nullptr;
    std::shared_ptr<Logger> _logger;

    VK_Shader _box_shader;
    VK_Shader _sphere_shader;
    VkDescriptorSetLayout _descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkPipeline _box_pipeline = VK_NULL_HANDLE;
    VkPipeline _sphere_pipeline = VK_NULL_HANDLE;
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;

    std::map<std::string, Held> _held;

    // the skies that could not be made, each said once and not tried again
    std::set<std::string> _failed;

    bool MakePipeline(const VK_Shader &shader, VkRenderPass render_pass, VkPipeline &pipeline) const;

    /// Loads the images of a sky and makes what its shader reads them
    /// through. Returns false when it cannot be made, which is logged.
    bool Load(const SkyInfo &sky, const std::string &key, Held &held);

    void Free(Held &held) const;

  public:
    static constexpr const char *kBox_Shader_Path = "engine://shaders/sky-box";
    static constexpr const char *kSphere_Shader_Path = "engine://shaders/sky-sphere";

    /// The bindings of the one set of both shaders, which have to match
    /// sky-box.frag and sky-sphere.frag: the images of the sky, a cube for
    /// the one and a panorama for the other, and the sampler they are read
    /// through, bound apart, see shaders.md.
    static constexpr uint32_t kImage_Binding = 0;
    static constexpr uint32_t kSampler_Binding = 1;

    static constexpr std::array<VkDescriptorSetLayoutBinding, 2> kBindings{{
      {kImage_Binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kSampler_Binding, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    }};

    /// How many skies are held at once: the one of the scene, and the one
    /// of the scene before it until the next frame.
    static constexpr uint32_t kMax_Skies = 8;

    /// Makes the pipelines for `render_pass`, which draws the scene image
    /// with its depth.
    bool Initialize(
      VK_Device *device,
      FileSystemContext *file_system_context,
      VkRenderPass render_pass,
      const VK_Samplers *samplers,
      const std::shared_ptr<Logger> &logger);

    void CleanUp();

    /// What tells two skies apart: the type, and the images it names. How
    /// it is turned and how bright it is are told to the shader, and need
    /// no images of their own.
    [[nodiscard]] static std::string KeyOf(const SkyInfo &sky);

    /// The images of a box in the order of the layers of a cube. A cube
    /// counts z the other way round than the world does, so `front`, seen
    /// along negative z, is the layer of positive z, and the shader turns
    /// the direction it reads by round to match.
    [[nodiscard]] static std::array<std::string, VK_Texture::kCube_Faces> FacesOf(const SkyInfo &sky);

    /// What the shader is told for a camera. `projection` is the one the
    /// scene is drawn with, with depth from 0 to 1.
    [[nodiscard]] static VK_SkyValues ValuesOf(const SkyInfo &sky, const glm::mat4 &view, const glm::mat4 &projection);

    /// What a sky is drawn with for a camera, its images loaded when it is
    /// drawn for the first time. Returns false when it cannot be drawn,
    /// which is logged once for a sky.
    bool Prepare(const SkyInfo &sky, const glm::mat4 &view, const glm::mat4 &projection, VK_SkyDraw &draw);

    /// Covers what is drawn to with the sky, wherever nothing is nearer.
    /// Call it in the render pass given to Initialize(), with its viewport
    /// set.
    void Draw(VkCommandBuffer commands, const VK_SkyDraw &draw) const;

    /// Frees the images of every sky that was not drawn since the last
    /// call. Call it between frames, when no commands refer to them.
    void ReleaseUnused();

    /// How many skies are held.
    [[nodiscard]] std::size_t Size() const { return _held.size(); }
  };
} // neon

#endif //VK_SKY_HPP
