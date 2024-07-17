#ifndef OPEN_GL_RENDER_SYSTEM_HPP
#define OPEN_GL_RENDER_SYSTEM_HPP

#include <vector>
#include "render-system.hpp"
#include "core/private/neon.hpp"

namespace core
{
  class OpenGl_RenderSystem final : public RenderSystem
  {
    const uint _max_buff_size = 65536;
    std::vector<std::tuple<GLuint, GLuint, GLuint, GLint>> _geometry_references;
    std::vector<GLuint> _shader_references;
    std::vector<GLuint> _texture_references;

  public:
    explicit OpenGl_RenderSystem(const SettingsConfig &settings_config)
      : RenderSystem(settings_config) {}

    void Initialize() override;

    void CleanUp() override;

    void RenderFrame() override;

    RenderContext* GetContext() override;

    int InitGeometry() override;

    void DestroyGeometry(int geometry_id) override;

    int InitShader() override;

    void DestroyShader(int shader_id) override;

    int InitTexture() override;

    void DestroyTexture(int texture_id) override;
  };
} // core

#endif //OPEN_GL_RENDER_SYSTEM_HPP
