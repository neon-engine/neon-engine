#ifndef SHAPE_HPP
#define SHAPE_HPP

#include <iostream>
#include <vector>
#include "GL/gl3w.h"

class Geometry
{
protected:
    std::string _id;
    GLuint _vao = 0;
    GLuint _vbo = 0;
    GLuint _ebo = 0;
    GLint _stride;

    std::vector<GLfloat> _vertices;
    std::vector<GLint> _indices;

    explicit Geometry(const std::string &id, const GLint &stride = 8 * sizeof(GLfloat))
    {
        _id = id;
        _stride = stride;
    }

    ~Geometry() = default;


    void Init()
    {
        std::cout << "Initializing mesh: " << _id << std::endl;

        glGenVertexArrays(1, &_vao);
        glGenBuffers(1, &_vbo);
        glGenBuffers(1, &_ebo);

        glBindVertexArray(_vao);

        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(_vertices.size() * sizeof(float)),
                     &_vertices[0],
                     GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(_indices.size() * sizeof(int)),
                     &_indices[0],
                     GL_STATIC_DRAW);

        // position
        glVertexAttribPointer(
            0,
            // "layout (location = x)" in the vertex shader
            3,
            // how many components there are per vertex
            GL_FLOAT,
            // What type these components are
            GL_FALSE,
            // GL_TRUE means the values should be normalized. GL_FALSE means they shouldn't
            _stride,
            static_cast<void *>(nullptr));
        glEnableVertexAttribArray(0);

        // colors
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            _stride,
            reinterpret_cast<void *>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // texture coordinates
        glVertexAttribPointer(
            2,
            2,
            GL_FLOAT,
            GL_FALSE,
            _stride,
            reinterpret_cast<void *>(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // We've sent the vertex data over to OpenGL, but there's still something missing.
        // In what order should it draw those vertices? That's why we'll need a GL_ELEMENT_ARRAY_BUFFER for this.


        // Unbind the currently bound buffer so that we don't accidentally make unwanted changes to it.
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Unbind the VAO now, so we don't accidentally tamper with it.
        // NOTE: You must NEVER unbind the element array buffer associated with a VAO!
        glBindVertexArray(0);
    }

    void CleanUp() const
    {
        std::cout << "cleaning up mesh: " << _id << std::endl;
        // Delete previously generated buffers. Note that forgetting to do this can waste GPU memory in a
        // large project! This could crash the graphics driver due to memory leaks, or slow down application performance!
        glDeleteVertexArrays(1, &_vao);
        glDeleteBuffers(1, &_vbo);
        glDeleteBuffers(1, &_ebo);
    }

public:
    virtual void Draw() const = 0;
};

#endif //SHAPE_HPP

#pragma clang diagnostic pop
