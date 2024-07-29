#ifndef CAMERA_NODE_HPP
#define CAMERA_NODE_HPP
#include "node.hpp"

namespace core {

class CameraNode final : public Node {
public:
  CameraNode(const std::string &name, const Transform &transform)
    : Node(name, transform) {}
};

} // core

#endif //CAMERA_NODE_HPP
