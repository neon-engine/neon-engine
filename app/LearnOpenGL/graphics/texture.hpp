#ifndef TEXTURE_HPP
#define TEXTURE_HPP

class Texture
{
private:
    GLuint _texture_id;
    static GLint MaxTextureUnits;

public:
    explicit Texture(const char* texture_path, GLenum format = GL_RGB);
    ~Texture();

    void Use(int unit = 0) const;
};

#endif //TEXTURE_HPP
