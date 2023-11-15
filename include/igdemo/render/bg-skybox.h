#ifndef IGDEMO_RENDER_BG_SKYBOX_H
#define IGDEMO_RENDER_BG_SKYBOX_H

#include <igasset/schema/igasset.h>
#include <igdemo/render/geo/cube.h>
#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp>

namespace igdemo {

struct BgSkyboxPipeline {
  struct CameraParamsGPU {
    glm::mat4 matView;
    glm::mat4 matProj;
  };

  static BgSkyboxPipeline Create(const wgpu::Device& device,
                                 const wgpu::Queue& queue,
                                 const IgAsset::WgslSource* wgsl,
                                 const wgpu::TextureView& cubemapView);
  BgSkyboxPipeline(wgpu::RenderPipeline pipeline_,
                   wgpu::BindGroupLayout cameraParamsBgl_,
                   wgpu::BindGroupLayout cubemapBgl_,
                   wgpu::Buffer cameraParamsBuffer_,
                   wgpu::BindGroup cameraParamsBg_, wgpu::BindGroup cubemapBg_)
      : pipeline(pipeline_),
        cameraParamsBgl(cameraParamsBgl_),
        cubemapBgl(cubemapBgl_),
        cameraParamsBuffer(cameraParamsBuffer_),
        cameraParamsBg(cameraParamsBg_),
        cubemapBg(cubemapBg_) {}

  wgpu::RenderPipeline pipeline;
  wgpu::BindGroupLayout cameraParamsBgl;
  wgpu::BindGroupLayout cubemapBgl;

  void set_camera_params(const wgpu::Queue& queue, const glm::mat4& matView,
                         const glm::mat4& matProj);
  void add_to_render_pass(const wgpu::RenderPassEncoder& pass,
                          const CubemapUnitCube& cube_geo);

 private:
  wgpu::Buffer cameraParamsBuffer;
  wgpu::BindGroup cameraParamsBg;
  wgpu::BindGroup cubemapBg;
};

}  // namespace igdemo

#endif
