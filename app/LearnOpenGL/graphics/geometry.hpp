#ifndef SHAPE_HPP
#define SHAPE_HPP


#include <vector>
#include "GL/gl3w.h"

class Geometry
{
protected:
    ~Geometry() = default;
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
    unsigned int _ebo = 0;

    std::vector<float> _vertices;
    std::vector<int> _indices;

    void Init()
    {
        // Create array object and buffers. Remember to delete your buffers when the object is destroyed!
        glGenVertexArrays(1, &_vao);
        glGenBuffers(1, &_vbo);
        glGenBuffers(1, &_ebo);

        // Bind the Vertex Array Object (VAO) first, then bind the associated buffers to it.
        // Consider the VAO as a container for all your buffers.
        glBindVertexArray(_vao);

        // Now bind a VBO to it as a GL_ARRAY_BUFFER. The GL_ARRAY_BUFFER is an array containing relevant data to what
        // you want to draw, such as vertices, normals, colors, etc.
        glBindBuffer(GL_ARRAY_BUFFER, _vbo);
        // glBufferData populates the most recently bound buffer with data starting at the 3rd argument and ending after
        // the 2nd argument number of indices. How does OpenGL know how long an index spans? Go to glVertexAttribPointer.
        glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), &_vertices[0], GL_STATIC_DRAW);
        // Enable the usage of layout location 0 (check the vertex shader to see what this is)
        glEnableVertexAttribArray(0);
        constexpr int stride = 8 * sizeof(float);

        // vertices
        glVertexAttribPointer(
            0,
            // This first parameter x should be the same as the number passed into the line "layout (location = x)" in the vertex shader. In this case, it's 0. Valid values are 0 to GL_MAX_UNIFORM_LOCATIONS.
            3,
            // This second line tells us how any components there are per vertex. In this case, it's 3 (we have an x, y, and z component)
            GL_FLOAT, // What type these components are
            GL_FALSE, // GL_TRUE means the values should be normalized. GL_FALSE means they shouldn't
            stride,
            // Offset between consecutive indices. Since each of our vertices have 3 floats, they should have the size of 3 floats in between
            static_cast<void*>(nullptr));
        // Offset of the first vertex's component. In our case it's 0 since we don't pad the vertices array with anything.
        glEnableVertexAttribArray(0);

        // colors
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            stride,
            reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // texture coordinates
        glVertexAttribPointer(
            2,
            2,
            GL_FLOAT,
            GL_FALSE,
            stride,
            reinterpret_cast<void*>(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        // We've sent the vertex data over to OpenGL, but there's still something missing.
        // In what order should it draw those vertices? That's why we'll need a GL_ELEMENT_ARRAY_BUFFER for this.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(int), &_indices[0], GL_STATIC_DRAW);

        // Unbind the currently bound buffer so that we don't accidentally make unwanted changes to it.
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Unbind the VAO now, so we don't accidentally tamper with it.
        // NOTE: You must NEVER unbind the element array buffer associated with a VAO!
        glBindVertexArray(0);
    }

    void CleanUp() const
    {
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
