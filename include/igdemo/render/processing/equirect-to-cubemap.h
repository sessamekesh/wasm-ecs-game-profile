#ifndef IGDEMO_RENDER_PROCESSING_EQUIRECT_TO_CUBEMAP_H
#define IGDEMO_RENDER_PROCESSING_EQUIRECT_TO_CUBEMAP_H

#include <igasset/schema/igasset.h>
#include <webgpu/webgpu_cpp.h>

namespace igdemo {

struct ConversionOutput {
  wgpu::Texture cubemap;
  wgpu::TextureView cubemapView;
};

class EquirectangularToCubemapPipeline {
 public:
  static EquirectangularToCubemapPipeline Create(
      const wgpu::Device& device, const IgAsset::WgslSource* wgsl_source);

  EquirectangularToCubemapPipeline(wgpu::RenderPipeline pipeline,
                                   wgpu::BindGroupLayout mvpBgl,
                                   wgpu::BindGroupLayout samplerBgl)
      : pipeline_(pipeline), mvpBgl_(mvpBgl), samplerBgl_(samplerBgl) {}

  ConversionOutput convert(const wgpu::Device& device, const wgpu::Queue& queue,
                           const wgpu::TextureView& equirect,
                           std::uint32_t cubemap_face_width,
                           wgpu::TextureUsage usage,
                           const char* label = nullptr);

 private:
  wgpu::RenderPipeline pipeline_;
  wgpu::BindGroupLayout mvpBgl_;
  wgpu::BindGroupLayout samplerBgl_;
};

}  // namespace igdemo

#endif
