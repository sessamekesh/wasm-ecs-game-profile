#ifndef IGDEMO_RENDER_PROCESSING_PREFILTER_ENV_GEN_H
#define IGDEMO_RENDER_PROCESSING_PREFILTER_ENV_GEN_H

#include <igasset/schema/igasset.h>
#include <igdemo/render/geo/cube.h>
#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

class PrefilteredEnvironmentMapGen {
 public:
  static PrefilteredEnvironmentMapGen Create(const wgpu::Device& device,
                                             const wgpu::Queue& queue,
                                             const IgAsset::WgslSource* wgsl);
  PrefilteredEnvironmentMapGen(wgpu::RenderPipeline pipeline,
                               wgpu::BindGroupLayout cameraParamsBgl,
                               wgpu::BindGroupLayout skyboxSamplerBgl,
                               wgpu::BindGroupLayout pbrParamsBgl,
                               wgpu::Buffer allCameraParamsBuffer,
                               wgpu::BindGroup cameraParamsBindGroups[6])
      : pipeline_(pipeline),
        cameraParamsBgl_(cameraParamsBgl),
        skyboxSamplerBgl_(skyboxSamplerBgl),
        pbrParamsBgl_(pbrParamsBgl),
        allCameraParamsBuffer_(allCameraParamsBuffer) {
    for (int i = 0; i < 6; i++) {
      cameraParamsBindGroups_[i] = cameraParamsBindGroups[i];
    }
  }

  TextureWithMeta generate(const wgpu::Device& device, const wgpu::Queue& queue,
                           const CubemapUnitCube& cube_geo,
                           const wgpu::TextureView& input_texture_view,
                           std::uint32_t input_texture_width,
                           std::uint32_t output_texture_width,
                           const char* label = nullptr) const;

 private:
  wgpu::RenderPipeline pipeline_;
  wgpu::BindGroupLayout cameraParamsBgl_;
  wgpu::BindGroupLayout skyboxSamplerBgl_;
  wgpu::BindGroupLayout pbrParamsBgl_;

  wgpu::Buffer allCameraParamsBuffer_;
  wgpu::BindGroup cameraParamsBindGroups_[6];
};

}  // namespace igdemo

#endif
