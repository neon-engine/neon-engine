#ifndef UTIL_HPP
#define UTIL_HPP

#include <iomanip>

#include <iostream>
#include <vector>
#include <glm/glm.hpp>

#include <glm/gtc/type_ptr.hpp>

namespace core
{
  static bool load_obj(
    const char *filename,
    std::vector<float> &vertices,
    std::vector<float> &normals,
    std::vector<float> &uvs,
    std::vector<int> &indices
    )
  {
    std::vector<unsigned int> indices_n;
    std::vector<unsigned int> indices_t;
    std::cout << "loading " << filename << std::endl;

    float x, y, z;
    float r, g, b;
    int fx, fy, fz;
    int fnx, fny, fnz;
    int ftx, fty, ftz;

    std::vector<glm::vec3> temp_n;
    std::vector<glm::vec2> temp_t;

    char line[BUFSIZ];

    FILE *fp = fopen(filename, "rb");
    if (fp == nullptr)
    {
      std::cerr << "error loading file" << std::endl;
      return false;
    }

    while (fgets(line, BUFSIZ, fp) != nullptr)
    {
      if (line[0] == 'v')
      {
        if (line[1] == ' ')
        {
          sscanf(line, "v %f %f %f %f %f %f", &x, &y, &z, &r, &g, &b);
          vertices.push_back(x);
          vertices.push_back(y);
          vertices.push_back(z);
        } else if (line[1] == 'n')
        {
          sscanf(line, "vn %f %f %f", &x, &y, &z);
          temp_n.push_back(glm::vec3(x, y, z)); // NOLINT(*-use-emplace)
        } else if (line[1] == 't')
        {
          sscanf(line, "vt %f %f", &x, &y);
          temp_t.push_back(glm::vec2(x, -y)); // NOLINT(*-use-emplace)
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
      uvs.push_back(temp.x);
      uvs.push_back(temp.y);
    }

    return true;
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

  static std::string get_file_extension(const std::string &filename)
  {
    if (const size_t dot_position = filename.find_last_of('.'); dot_position != std::string::npos) {
      return filename.substr(dot_position + 1);
    }
    return "";
  }
}

#endif //UTIL_HPP
