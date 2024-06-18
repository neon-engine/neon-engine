#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#define STB_IMAGE_IMPLEMENTATION

#include <GL/gl3w.h>
#include <iostream>
#include "texture.hpp"
#include "stb_image.h"


Texture::Texture(const char* texture_path)
{
    int tex_width, tex_height, nr_channels;
    stbi_set_flip_vertically_on_load(true);
    if (unsigned char* data = stbi_load(
        texture_path,
        &tex_width,
        &tex_height,
        &nr_channels,
        STBI_rgb))
    {
        glGenTextures(1, &_texture_id);
        glBindTexture(GL_TEXTURE_2D, _texture_id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGB,
                     tex_width,
                     tex_height,
                     0,
                     GL_RGB,
                     GL_UNSIGNED_BYTE,
                     data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // free the memory after we are done creating texture
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
        _texture_id = 0;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture::~Texture()
{
    if (_texture_id)
    {
        glDeleteTextures(1, &_texture_id);
    }
}

void Texture::Use(const int unit) const
{
    int maxTextureUnits;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextureUnits);

    if (unit >= 0 && unit < maxTextureUnits) {
        const GLenum textureUnit = GL_TEXTURE0 + unit;
        glActiveTexture(textureUnit);
        std::cout << "Activated texture unit: " << unit << std::endl;
    } else {
        std::cerr << "Error: Texture unit " << unit << " is out of range. Supported range: 0 to " << (maxTextureUnits - 1) << std::endl;
    }
    glBindTexture(GL_TEXTURE_2D, _texture_id);
}

#pragma clang diagnostic pop
