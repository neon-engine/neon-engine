#ifndef UTIL_HPP
#define UTIL_HPP

#include <fstream>
#include <iomanip>

#include <iostream>
#include <map>
#include <vector>
#include <glm/glm.hpp>

#include <glm/gtc/type_ptr.hpp>

namespace core
{
  // ReSharper disable once CppDFAUnreachableFunctionCall
  static std::vector<std::string> split(const std::string &s, const char delimiter)
  {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream token_stream(s);
    while (std::getline(token_stream, token, delimiter))
    {
      tokens.push_back(token);
    }
    return tokens;
  }

  static bool load_obj(
    const std::string &filename,
    std::vector<float> &vertices,
    std::vector<float> &normals,
    std::vector<float> &uvs,
    std::vector<unsigned int> &indices
  )
  {
    std::ifstream obj_file(filename);
    if (!obj_file.is_open())
    {
      std::cerr << "Error opening file" << std::endl;
      return false;
    }

    std::vector<float> temp_vert;
    std::vector<float> temp_uv;
    std::vector<float> temp_normals;

    std::map<std::string, int> index_map;
    int current_index = 0;

    std::string line;
    while (std::getline(obj_file, line))
    {
      std::istringstream line_stream(line);
      std::string prefix;
      line_stream >> prefix;

      if (prefix == "v")
      {
        float x, y, z;
        line_stream >> x >> y >> z;
        temp_vert.push_back(x);
        temp_vert.push_back(y);
        temp_vert.push_back(z);
      } else if (prefix == "vt")
      {
        float u, v;
        line_stream >> u >> v;
        temp_uv.push_back(u);
        temp_uv.push_back(v);
      } else if (prefix == "vn")
      {
        float nx, ny, nz;
        line_stream >> nx >> ny >> nz;
        temp_normals.push_back(nx);
        temp_normals.push_back(ny);
        temp_normals.push_back(nz);
      } else if (prefix == "f")
      {
        std::string vertex_data;
        while (line_stream >> vertex_data)
        {
          if (index_map.contains(vertex_data))
          {
            indices.push_back(index_map[vertex_data]);
          } else
          {
            auto indices_parts = split(vertex_data, '/');
            int vert_index = (std::stoi(indices_parts[0]) - 1) * 3;
            int tex_index = (std::stoi(indices_parts[1]) - 1) * 2;
            int norm_index = (std::stoi(indices_parts[2]) - 1) * 3;

            auto x = temp_vert[vert_index];
            auto y = temp_vert[vert_index + 1];
            auto z = temp_vert[vert_index + 2];

            auto u = temp_uv[tex_index];
            auto v = temp_uv[tex_index + 1];

            auto nx = temp_normals[norm_index];
            auto ny = temp_normals[norm_index + 1];
            auto nz = temp_normals[norm_index + 2];

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);

            uvs.push_back(u);
            uvs.push_back(v);

            index_map[vertex_data] = current_index;

            indices.push_back(current_index);
            current_index++;
          }
        }
      }
    }
    return true;
  }

  static std::string mat4_to_string(const glm::mat4 &matrix)
  {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    const float *m = value_ptr(matrix);

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
    if (const size_t dot_position = filename.find_last_of('.'); dot_position != std::string::npos)
    {
      return filename.substr(dot_position + 1);
    }
    return "";
  }
}

#endif //UTIL_HPP
