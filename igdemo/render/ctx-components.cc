#include <igdemo/render/ctx-components.h>
#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

CtxGeneral3dBuffers::CtxGeneral3dBuffers(const wgpu::Device& device,
                                         const wgpu::Queue& queue)
    : cameraBuffer(nullptr), lightingBuffer(nullptr) {
  cameraBuffer = create_buffer(
      device, queue, CtxAnimatedPbrPipeline::GPUCameraParams{},
      wgpu::BufferUsage::Uniform, "common-3d-camera-params-buffer");
  lightingBuffer = create_buffer(
      device, queue, CtxAnimatedPbrPipeline::GPULightingParams{},
      wgpu::BufferUsage::Uniform, "common-3d-lighting-params-buffer");
}

void CtxGeneral3dBuffers::update_camera(
    const wgpu::Queue& queue,
    const CtxAnimatedPbrPipeline::GPUCameraParams& params) {
  queue.WriteBuffer(cameraBuffer, 0, &params,
                    sizeof(CtxAnimatedPbrPipeline::GPUCameraParams));
}

void CtxGeneral3dBuffers::update_lighting(
    const wgpu::Queue& queue,
    const CtxAnimatedPbrPipeline::GPULightingParams& params) {
  queue.WriteBuffer(cameraBuffer, 0, &params,
                    sizeof(CtxAnimatedPbrPipeline::GPULightingParams));
}

CtxHdrPassOutput::CtxHdrPassOutput(const wgpu::Device& device, std::uint32_t w_,
                                   std::uint32_t h_)
    : hdrColorTexture(nullptr),
      hdrColorTextureView(nullptr),
      dsTexture(nullptr),
      dsView(nullptr),
      width(w_),
      height(h_) {
  wgpu::TextureDescriptor hdr_desc{};
  hdr_desc.format = wgpu::TextureFormat::RGBA32Float;
  hdr_desc.dimension = wgpu::TextureDimension::e2D;
  hdr_desc.label = "igdemo-hdr-render-target";
  hdr_desc.mipLevelCount = 1;
  hdr_desc.size = {width, height, 1};
  hdr_desc.usage =
      wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;

  hdrColorTexture = device.CreateTexture(&hdr_desc);
  hdrColorTextureView = hdrColorTexture.CreateView();

  wgpu::TextureDescriptor ds_desc{};
  ds_desc.dimension = wgpu::TextureDimension::e2D;
  ds_desc.format = wgpu::TextureFormat::Depth32Float;
  ds_desc.label = "final-frame-depth-texture";
  ds_desc.size = {width, height};
  ds_desc.usage = wgpu::TextureUsage::RenderAttachment;
  dsTexture = device.CreateTexture(&ds_desc);
  dsView = dsTexture.CreateView();
}

}  // namespace igdemo
