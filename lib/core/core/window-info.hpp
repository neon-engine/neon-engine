#ifndef WINDOW_INFO_HPP
#define WINDOW_INFO_HPP
#include <string>

enum class SupportedApi
{
  OpenGl = 0
};

struct Window_Info
{
  int width;
  int height;
  std::string title;
  SupportedApi supported_api;
};

#endif //WINDOW_INFO_HPP
