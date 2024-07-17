#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

namespace core
{
  class RenderContext
  {
  protected:
    int _geometry_index = 0;
    int _shader_index = 0;
    int _texture_index = 0;

    ~RenderContext() = default;

  public:
    virtual int InitGeometry() = 0;

    virtual void DestroyGeometry(int geometry_id) = 0;

    virtual int InitShader() = 0;

    virtual void DestroyShader(int shader_id) = 0;

    virtual int InitTexture() = 0;

    virtual void DestroyTexture(int texture_id) = 0;
  };
} // core

#endif //RENDER_CONTEXT_HPP
