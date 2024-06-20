#ifndef WINDOW_INFO_HPP
#define WINDOW_INFO_HPP
#include <string>

enum class RenderingApi
{
  OpenGl = 0
};

struct SettingsConfig
{
  int width;
  int height;
  std::string title;
  RenderingApi selected_api;
};

#endif //WINDOW_INFO_HPP
