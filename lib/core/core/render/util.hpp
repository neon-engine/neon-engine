#ifndef UTIL_HPP
#define UTIL_HPP

#include <iomanip>
#include <GL/gl3w.h>

#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#include "glm/gtc/type_ptr.hpp"

namespace core
{
  static void load_obj(const char *filename)
  {
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLuint> indices;
    std::vector<GLuint> indices_n;
    std::vector<GLuint> indices_t;
    std::vector<GLfloat> uv;
    std::cout << "loading " << filename << std::endl;

    float x, y, z;
    float r, g, b;
    int fx, fy, fz;
    int fnx, fny, fnz;
    int ftx, fty, ftz;

    std::vector<glm::vec3> temp_v;
    std::vector<glm::vec3> temp_n;
    std::vector<glm::vec2> temp_t;

    char line[BUFSIZ];

    FILE *fp = fopen(filename, "rb");
    if (fp == nullptr)
    {
      std::cerr << "error loading file" << std::endl;
      exit(-1);
    }
    int curr = 0;

    while (fgets(line, BUFSIZ, fp) != nullptr)
    {
      if (line[0] == 'v')
      {
        if (line[1] == ' ')
        {
          sscanf(line, "v %f %f %f %f %f %f", &x, &y, &z, &r, &g, &b);
          temp_v.push_back(glm::vec3(x, y, z));
        } else if (line[1] == 'n')
        {
          sscanf(line, "vn %f %f %f", &x, &y, &z);
          temp_n.push_back(glm::vec3(x, y, z));
        } else if (line[1] == 't')
        {
          sscanf(line, "vt %f %f", &x, &y);
          temp_t.push_back(glm::vec2(x, -y));
        }
      }
      if (line[0] == 'f')
      {
        if (line[1] == ' ')
        {
          sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &fx, &ftx, &fnx, &fy, &fty, &fny, &fz, &ftz, &fnz);
          indices.push_back(fx - 1);
          indices.push_back(fy - 1);
          indices.push_back(fz - 1);

          indices_n.push_back(fnx - 1);
          indices_n.push_back(fny - 1);
          indices_n.push_back(fnz - 1);

          indices_t.push_back(ftx - 1);
          indices_t.push_back(fty - 1);
          indices_t.push_back(ftz - 1);
        }
      }
    }

    for (auto vertex : indices)
    {
      auto temp = temp_v[vertex];

      vertices.push_back(temp.x);
      vertices.push_back(temp.y);
      vertices.push_back(temp.z);
    }

    for (auto normal : indices_n)
    {
      auto temp = temp_n[normal];

      normals.push_back(temp.x);
      normals.push_back(temp.y);
      normals.push_back(temp.z);
    }

    for (auto index : indices_t)
    {
      auto temp = temp_t[index];
      uv.push_back(temp.x);
      uv.push_back(temp.y);
    }

    std::cout << "vertices: " << vertices.size() << std::endl;
    for (auto i = 0; i < vertices.size(); i += 3)
    {
      std::cout << vertices[i] << "f, " << vertices[i + 1] << "f, " << vertices[i + 2] << "f," << std::endl;
    }
    std::cout << "normals: " << normals.size() << std::endl;
    for (auto i = 0; i < normals.size(); i += 3)
    {
      std::cout << normals[i] << "f, " << normals[i + 1] << "f, " << normals[i + 2] << "f," << std::endl;
    }
    std::cout << "uvs: " << uv.size() << std::endl;
    for (auto i = 0; i < uv.size(); i += 2)
    {
      std::cout << uv[i] << "f, " << uv[i + 1] << "f," << std::endl;
    }
    std::cout << "indices: " << indices.size() << std::endl;
    for (auto i = 0; i < indices.size(); i += 3)
    {
      std::cout << indices[i] << ", " << indices[i + 1] << ", " << indices[i + 2] << "," << std::endl;
    }
  }

  static std::string mat4_to_string(const glm::mat4 &matrix)
  {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    const float *m = glm::value_ptr(matrix);

    for (int row = 0; row < 4; ++row)
    {
      for (int col = 0; col < 4; ++col)
      {
        oss << m[col * 4 + row] << " ";
      }
      oss << "\n";
    }

    return oss.str();
  }
}

#endif //UTIL_HPP
