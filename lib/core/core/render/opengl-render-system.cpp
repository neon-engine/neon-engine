#include "opengl-render-system.hpp"



#include <fstream>
#include <iostream>
#include <sstream>

#include <stdexcept>
#include <GL/gl3w.h>


namespace core
{
  void OpenGL_RenderSystem::Initialize()
  {
    std::cout << "Initializing OpenGL" << std::endl;
    if (gl3wInit())
    {
      throw std::runtime_error("Failed to initialize gl3w");
    }

    glEnable(GL_DEPTH_TEST);

    int num_vertex_attributes_supported;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &num_vertex_attributes_supported);
    std::cout << "Maximum number of vertex attributes supported: " << num_vertex_attributes_supported << std::endl;

    glViewport(0, 0, _settings_config.width, _settings_config.height);

    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &_max_texture_units);
  }

  void OpenGL_RenderSystem::CleanUp()
  {
    std::cout << "Cleaning up OpenGL" << std::endl;
  }

  void OpenGL_RenderSystem::PrepareFrame()
  {
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); for wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  RenderContext *OpenGL_RenderSystem::GetContext()
  {
    return this;
  }

  int OpenGL_RenderSystem::InitMesh(
    const std::vector<float> &vertices,
    const std::vector<float> &normals,
    const std::vector<float> &tex_coordinates,
    const std::vector<int> &indices)
  {
    const auto geometry_id = _geometry_index++;
    std::cout << "Initializing mesh: " << geometry_id << std::endl;

    if (geometry_id >= _geometry_references.size())
    {
      throw std::runtime_error("geometry id exceeded geometry db size");
    }

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint nvbo = 0;
    GLuint uvbo = 0;
    GLuint ebo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &nvbo);
    glGenBuffers(1, &uvbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    // vbo - vertices
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
      GL_ARRAY_BUFFER,
      static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
      &vertices[0],
      GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      0,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(GLfloat),
      static_cast<void *>(nullptr));

    // nvbo - normals
    glBindBuffer(GL_ARRAY_BUFFER, nvbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(normals.size() * sizeof(float)),
                 &normals[0],
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
      1,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(GLfloat),
      static_cast<void *>(nullptr));

    // uvbo - texture coordinates
    glBindBuffer(GL_ARRAY_BUFFER, uvbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(tex_coordinates.size() * sizeof(float)),
                 &tex_coordinates[0],
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
      2,
      2,
      GL_FLOAT,
      GL_FALSE,
      2 * sizeof(GLfloat),
      static_cast<void *>(nullptr));


    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(int)),
                 &indices[0],
                 GL_STATIC_DRAW);

    // Unbind the currently bound buffer so that we don't accidentally make unwanted changes to it.
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind the VAO now, so we don't accidentally tamper with it.
    // NOTE: You must NEVER unbind the element array buffer associated with a VAO!
    glBindVertexArray(0);

    _geometry_references[geometry_id] = OpenGL_Geometry{
      .vao = vao,
      .vbo = vbo,
      .vertices_size = vertices.size(),
      .nvbo = nvbo,
      .normals_size = normals.size(),
      .uvbo = uvbo,
      .uvs_size = tex_coordinates.size(),
      .ebo = ebo,
      .indices_size = indices.size()
    };

    return geometry_id;
  }

  void OpenGL_RenderSystem::DrawMesh(const int geometry_id)
  {
    // ReSharper disable once CppUseStructuredBinding
    const auto geometry = _geometry_references[geometry_id];
    if (geometry.vao == 0)
    {
      std::cerr << "Attempted to draw geometry id " << geometry_id << " but it is currently not loaded" << std::endl;
      return;
    }

    glBindVertexArray(geometry.vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(geometry.indices_size), GL_UNSIGNED_INT, nullptr);
  }


  void OpenGL_RenderSystem::DestroyMesh(const int geometry_id)
  {
    std::cout << "Destroying geometry with id " << geometry_id << std::endl;

    // ReSharper disable once CppUseStructuredBinding
    const auto geometry = _geometry_references[geometry_id];



    _geometry_references[geometry_id] = OpenGL_Geometry{};
  }

  int OpenGL_RenderSystem::InitMaterial(const std::string shader_path, std::string texture_paths[])
  {
    const auto material_id = _material_index++;
    std::cout << "Initializing material with id " << material_id << std::endl;
    const auto shader_id = InitShader(shader_path);
    std::vector<GLuint> texture_references;

    glUseProgram(shader_id);
    for(auto i = 0; i < texture_paths->length(); i++)
    {
      texture_references.push_back(InitTexture(texture_paths[i]));
      std::string texture_str = "texture";
      texture_str += std::to_string(i);
      glUniform1i(glGetUniformLocation(shader_id, texture_str.c_str()), i);
    }
    glUseProgram(0);

    const auto material = OpenGL_Material{
      .shader_program_id = shader_id,
      .texture_ref_size = texture_references.size(),
      .texture_references = texture_references.data()
    };

    _material_references[material_id] = material;

    return material_id;
  }

  void OpenGL_RenderSystem::UseMaterial(const int material_id)
  {
    // ReSharper disable once CppUseStructuredBinding
    const auto material = _material_references[material_id];
    if (material.shader_program_id == 0)
    {
      std::cerr << "Attempted to use material id " << material_id << " but it is currently not loaded" << std::endl;
      return;
    }

    for (auto i = 0; i < material.texture_ref_size; i++)
    {
      const auto texture_id = material.texture_references[i];
      const GLint unit = i;
      // todo: implement queue based ids
      // currently this only works with a single textured object in the scene
      // we need to save the state
      const GLenum texture_unit = GL_TEXTURE0 + unit;
      glActiveTexture(texture_unit);
      glBindTexture(GL_TEXTURE_2D, texture_id);
    }
    glUseProgram(material.shader_program_id);
  }

  void OpenGL_RenderSystem::DestroyMaterial(int material_id) {}
} // core
