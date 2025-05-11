#ifndef WINDOW_INFO_HPP
#define WINDOW_INFO_HPP

#include <string>

enum class RenderingApi
{
  OpenGl = 0,
  Vulkan = 1
};

struct SettingsConfig
{
  int width;
  int height;
  std::string title;
  RenderingApi selected_api;
  std::string logpath = "logs/neon-engine.log";
  std::size_t log_max_size = 1048576 * 5;
  std::size_t log_max_files = 1;
  std::size_t max_light_sources = 1024;
};

#endif //WINDOW_INFO_HPP
