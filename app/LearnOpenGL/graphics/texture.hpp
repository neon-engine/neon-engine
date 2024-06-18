#ifndef TEXTURE_HPP
#define TEXTURE_HPP

class Texture
{
private:
    unsigned int _texture_id;

public:
    explicit Texture(const char* texture_path);
    ~Texture();

    void Use(int unit = 0) const;
};

#endif //TEXTURE_HPP
