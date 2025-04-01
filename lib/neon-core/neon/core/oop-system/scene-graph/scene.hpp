#ifndef SCENE_HPP
#define SCENE_HPP
#include <string>

#include "node/node.hpp"

namespace neon
{
  class Scene
  {
    Node _root;
  public:
    void Initialize(std::string filepath);
  };
} // neon

#endif //SCENE_HPP
