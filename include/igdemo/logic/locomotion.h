#ifndef IGDEMO_LOGIC_LOCOMOTION_H
#define IGDEMO_LOGIC_LOCOMOTION_H

#include <glm/glm.hpp>

namespace igdemo {

struct PositionComponent {
  glm::vec2 map_position;
};

struct OrientationComponent {
  float radAngle;
};

struct ScaleComponent {
  float scale;
};

}  // namespace igdemo

#endif
