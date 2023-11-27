#ifndef IGDEMO_RENDER_CAMERA_H
#define IGDEMO_RENDER_CAMERA_H

#include <igecs/world_view.h>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace igdemo {

struct CameraComponent {
  glm::vec3 position;
  float theta;
  float phi;

  float fovy;
  float nearPlaneDistance;
  float farPlaneDistance;
};

struct CtxActiveCamera {
  entt::entity activeCameraEntity;
};

}  // namespace igdemo

#endif
