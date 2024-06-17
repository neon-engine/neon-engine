#ifndef SHADER_HPP
#define SHADER_HPP

#include <string>

class Shader
{
private:
    unsigned int _program_id;

public:
    // constructor reads and builds the shader
    Shader(const char* vertex_path, const char* fragment_path);
    ~Shader();

    void Use() const;
    // utility uniform functions
    void SetBool(const std::string& name, bool value) const;
    void SetInt(const std::string& name, int value) const;
    void SetFloat(const std::string& name, float value) const;
};

#endif //SHADER_HPP
