#ifndef IGDEMO_RENDER_PROCESSING_IRRADIANCE_MAP_GENERATOR_H
#define IGDEMO_RENDER_PROCESSING_IRRADIANCE_MAP_GENERATOR_H

#include <igasset/schema/igasset.h>
#include <igdemo/render/geo/cube.h>
#include <igdemo/render/processing/gen-mips.h>
#include <igdemo/render/wgpu-helpers.h>

#include <glm/glm.hpp>

namespace igdemo {

struct IrradianceCubemap {
  TextureWithMeta irradianceCubemap;
};

struct IrradianceMapGenerator {
  static IrradianceMapGenerator Create(const wgpu::Device& device,
                                       const wgpu::Queue& queue,
                                       const IgAsset::WgslSource* wgsl);
  IrradianceMapGenerator(wgpu::RenderPipeline pipeline,
                         wgpu::BindGroupLayout cameraParamsBgl,
                         wgpu::BindGroupLayout samplerBgl,
                         wgpu::Buffer cameraParamsBuffer,
                         wgpu::BindGroup cameraParamsBindGroups[6])
      : pipeline_(pipeline),
        cameraParamsBgl_(cameraParamsBgl),
        samplerBgl_(samplerBgl),
        cameraParamsBuffer_(cameraParamsBuffer),
        cameraParamsBindGroups_() {
    for (int i = 0; i < 6; i++) {
      cameraParamsBindGroups_[i] = cameraParamsBindGroups[i];
    }
  }

  IrradianceCubemap generate(const wgpu::Device& device,
                             const wgpu::Queue& queue,
                             const CubemapUnitCube& cube_geo,
                             const HdrMipsGenerator& mips_generator,
                             const wgpu::TextureView& input_texture_view,
                             std::uint32_t texture_width,
                             const char* label = nullptr) const;

 private:
  wgpu::RenderPipeline pipeline_;
  wgpu::BindGroupLayout cameraParamsBgl_;
  wgpu::BindGroupLayout samplerBgl_;

  wgpu::Buffer cameraParamsBuffer_;
  wgpu::BindGroup cameraParamsBindGroups_[6];
};

}  // namespace igdemo

#endif
