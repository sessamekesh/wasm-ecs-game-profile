#ifndef IGDEMO_RENDER_PBR_COMMON_H
#define IGDEMO_RENDER_PBR_COMMON_H

#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>

namespace igdemo::pbr {

struct GPUCameraParams {
  glm::mat4 matViewProj;
  glm::vec3 cameraPos;
  float __padding_1__ = 0.f;
};

struct GPULightingParams {
  glm::vec3 sunColor;
  float ambientCoefficient;
  glm::vec3 sunDirection;
  float __padding_1__ = 0.f;
};

struct GPUPbrColorParams {
  glm::vec3 albedo;
  float metallic;
  float roughness;
  float __padding__[3] = {0.f, 0.f, 0.f};
};

}  // namespace igdemo::pbr

#endif
