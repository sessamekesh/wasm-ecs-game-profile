#ifndef IGDEMO_RENDER_PROCESSING_BRDFLUT_H
#define IGDEMO_RENDER_PROCESSING_BRDFLUT_H

#include <igasset/schema/igasset.h>
#include <igdemo/render/geo/fullscreen_quad.h>
#include <igdemo/render/wgpu-helpers.h>
#include <webgpu/webgpu_cpp.h>

namespace igdemo {

struct CtxBrdfLut {
  TextureWithMeta lut;
};

struct BRDFLutGenerator {
 public:
  static BRDFLutGenerator Create(const wgpu::Device& device,
                                 const IgAsset::WgslSource* wgsl);

  BRDFLutGenerator(wgpu::RenderPipeline pipeline) : pipeline_(pipeline) {}
  TextureWithMeta generate(const wgpu::Device& device, const wgpu::Queue& queue,
                           const FullscreenQuad& quad_geo,
                           std::uint32_t lut_width,
                           const char* label = nullptr) const;

 private:
  wgpu::RenderPipeline pipeline_;
};

}  // namespace igdemo

#endif
