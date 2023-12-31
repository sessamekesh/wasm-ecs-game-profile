#ifndef IGDEMO_RENDER_WORLD_TRANSFORM_COMPONENT_H
#define IGDEMO_RENDER_WORLD_TRANSFORM_COMPONENT_H

#include <ozz/animation/runtime/skeleton.h>

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace igdemo {

struct WorldTransformComponent {
  glm::mat4 worldTransform;
};

struct SkinComponent {
  // References to a piece of geometry externally (e.g. AnimatedPbrGeomety)
  const std::vector<std::string>* boneNames;
  const std::vector<glm::mat4>* invBindPoses;
  const ozz::animation::Skeleton* skeleton;

  std::vector<glm::mat4> skin;
};

}  // namespace igdemo

#endif
