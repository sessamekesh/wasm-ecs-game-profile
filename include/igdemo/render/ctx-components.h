#ifndef IGDEMO_RENDER_CTX_COMPONENTS_H
#define IGDEMO_RENDER_CTX_COMPONENTS_H

#include <igdemo/render/animated-pbr.h>
#include <webgpu/webgpu_cpp.h>

namespace igdemo {

struct CtxWgpuDevice {
  wgpu::Device device;
  wgpu::Queue queue;
  wgpu::TextureFormat renderTargetFormat;
  wgpu::TextureView renderTarget;
};

struct CtxGeneral3dBuffers {
  CtxGeneral3dBuffers(const wgpu::Device& device, const wgpu::Queue& queue);
  void update_camera(
      const wgpu::Queue& queue,
      const CtxAnimatedPbrPipeline::GPUCameraParams& camera_params);
  void update_lighting(
      const wgpu::Queue& queue,
      const CtxAnimatedPbrPipeline::GPULightingParams& lighting_params);

  wgpu::Buffer cameraBuffer;
  wgpu::Buffer lightingBuffer;
};

struct CtxHdrPassOutput {
  CtxHdrPassOutput(const wgpu::Device& device, std::uint32_t width,
                   std::uint32_t height);

  wgpu::Texture hdrColorTexture;
  wgpu::TextureView hdrColorTextureView;
  wgpu::Texture dsTexture;
  wgpu::TextureView dsView;
  std::uint32_t width;
  std::uint32_t height;
};

struct CtxGeneralSceneParams {
  glm::vec3 sunDirection;
  glm::vec3 sunColor;
  float ambientCoefficient;
};

}  // namespace igdemo

#endif
