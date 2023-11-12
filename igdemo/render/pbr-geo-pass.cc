#include <igdemo/render/ctx-components.h>
#include <igdemo/render/pbr-geo-pass.h>
#include <igdemo/render/world-transform-component.h>

namespace igdemo {

const igecs::WorldView::Decl& PbrUploadPerInstanceBuffersSystem::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           // Define outside of system
                                           .ctx_reads<CtxWgpuDevice>()

                                           // Iterators (external)
                                           .reads<SkinComponent>()
                                           .reads<WorldTransformComponent>()
                                           .writes<AnimatedPbrSkinBindGroup>();

  return decl;
}

void PbrUploadPerInstanceBuffersSystem::run(igecs::WorldView* wv) {
  const auto& ctxDevice = wv->ctx<CtxWgpuDevice>();
  const auto& queue = ctxDevice.queue;

  auto view = wv->view<const SkinComponent, const WorldTransformComponent,
                       AnimatedPbrSkinBindGroup>();

  for (auto [e, skin, worldTransform, buffers] : view.each()) {
    buffers.update(queue, skin.skin, worldTransform.worldTransform);
  }
}

bool PbrGeoPassSystem::setup_animated(
    const wgpu::Device& device, igecs::WorldView* wv,
    const igasset::IgpackDecoder& animated_pbr_decoder,
    const std::string& animated_pbr_igasset_name) {
  auto pipeline = CtxAnimatedPbrPipeline::Create(
      animated_pbr_decoder, animated_pbr_igasset_name, device);

  if (!pipeline) {
    // TODO (sessamekesh): Report back error state
    return false;
  }

  // Requires common parameters to be set up first
  const auto& generalBuffers = wv->ctx<CtxGeneral3dBuffers>();

  auto& ctxPipeline =
      wv->attach_ctx<CtxAnimatedPbrPipeline>(*std::move(pipeline));
  wv->attach_ctx<AnimatedPbrFrameBindGroup>(device, ctxPipeline.frame_bgl,
                                            generalBuffers.cameraBuffer,
                                            generalBuffers.lightingBuffer);

  return true;
}

const igecs::WorldView::Decl& PbrGeoPassSystem::decl() {
  static igecs::WorldView::Decl decl =
      igecs::WorldView::Decl()
          // DEFINE OUTSIDE OF SYSTEM
          .ctx_reads<CtxSceneLightingParams>()
          .ctx_reads<CtxWgpuDevice>()
          .ctx_writes<CtxGeneral3dBuffers>()
          .ctx_writes<CtxHdrPassOutput>()

          // DEFINED BY SYSTEM
          .ctx_reads<CtxAnimatedPbrPipeline>()
          .ctx_reads<AnimatedPbrFrameBindGroup>()

          // COMPONENT ITERATORS
          .reads<AnimatedPbrInstance>()
          .reads<SkinComponent>();
}

void PbrGeoPassSystem::run(igecs::WorldView* wv) {
  const auto& ctxWgpuDevice = wv->ctx<CtxWgpuDevice>();
  const auto& sceneLightingParams = wv->ctx<CtxSceneLightingParams>();
  auto& generalBuffers = wv->mut_ctx<CtxGeneral3dBuffers>();
  const auto& hdrOutView = wv->ctx<CtxHdrPassOutput>().hdrColorTextureView;
  const auto& dsView = wv->ctx<CtxHdrPassOutput>().dsView;
  const auto& ctxPipeline = wv->ctx<CtxAnimatedPbrPipeline>();
  const auto& ctxAnimatedPbrBindGroup = wv->ctx<AnimatedPbrFrameBindGroup>();

  const auto& device = ctxWgpuDevice.device;
  const auto& queue = ctxWgpuDevice.queue;

  generalBuffers.update_camera(queue, sceneLightingParams.cameraParams);
  generalBuffers.update_lighting(queue, sceneLightingParams.lightingParams);

  wgpu::RenderPassColorAttachment ca{};
  ca.clearValue = {0.f, 0.f, 0.f, 1.f};
  ca.loadOp = wgpu::LoadOp::Clear;
  ca.storeOp = wgpu::StoreOp::Store;
  ca.view = hdrOutView;

  wgpu::RenderPassDepthStencilAttachment ds{};
  ds.depthClearValue = 1.f;
  ds.depthLoadOp = wgpu::LoadOp::Clear;
  ds.depthStoreOp = wgpu::StoreOp::Store;
  ds.view = dsView;

  wgpu::RenderPassDescriptor rpd{};
  rpd.colorAttachmentCount = 1;
  rpd.colorAttachments = &ca;
  rpd.depthStencilAttachment = &ds;

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
  {
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&rpd);

    pass.SetPipeline(ctxPipeline.pipeline);
    pass.SetBindGroup(0, ctxAnimatedPbrBindGroup.frameBindGroup);

    {
      auto view =
          wv->view<const AnimatedPbrInstance, const AnimatedPbrSkinBindGroup>();

      for (auto [e, instance, bg] : view.each()) {
        const auto* geo = instance.geometry;
        const auto* mat = instance.material;

        pass.SetVertexBuffer(0, geo->vertexBuffer, 0, geo->vertexBufferSize);
        pass.SetVertexBuffer(1, geo->boneWeightsBuffer, 0,
                             geo->boneWeightsBufferSize);
        pass.SetIndexBuffer(geo->indexBuffer, geo->indexFormat, 0,
                            geo->indexBufferSize);
        pass.SetBindGroup(1, mat->objBindGroup);
        pass.SetBindGroup(2, bg.skinBindGroup);
        pass.DrawIndexed(geo->numIndices);
      }
    }

    pass.End();
  }
  wgpu::CommandBuffer commands = encoder.Finish();
  queue.Submit(1, &commands);
}

}  // namespace igdemo
