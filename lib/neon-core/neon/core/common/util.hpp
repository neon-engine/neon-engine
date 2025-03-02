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

  static float scale(const float val, glm::vec2 src, glm::vec2 dst) {
    return ((val - src[0]) / (src[1] - src[0])) * (dst[1] - dst[0]) + dst[0];
  }
}

#endif //UTIL_HPP
