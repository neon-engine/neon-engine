#ifndef SCENE_HPP
#define SCENE_HPP
#include <string>

#include "node/node.hpp"

namespace core
{
  class Scene
  {
    Node _root;
  public:
    void Initialize(std::string filepath);
  };
} // core

#endif //SCENE_HPP
