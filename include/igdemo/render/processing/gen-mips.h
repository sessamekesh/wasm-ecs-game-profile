#ifndef IGDEMO_RENDER_PROCESSING_GEN_MIPS_H
#define IGDEMO_RENDER_PROCESSING_GEN_MIPS_H

#include <igasset/schema/igasset.h>
#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

class HdrMipsGenerator {
 public:
  static HdrMipsGenerator Create(const wgpu::Device& device,
                                 const IgAsset::WgslSource* wgsl);
  HdrMipsGenerator(wgpu::ComputePipeline pipeline, wgpu::BindGroupLayout bgl)
      : pipeline_(pipeline), bgl_(bgl) {}

  void gen_cube_mips(const wgpu::Device& device, const wgpu::Queue& queue,
                     const wgpu::Texture& cubemapTexture,
                     std::uint32_t width) const;

 private:
  wgpu::ComputePipeline pipeline_;
  wgpu::BindGroupLayout bgl_;
};

}  // namespace igdemo

#endif
