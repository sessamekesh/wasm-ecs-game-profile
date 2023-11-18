#include <igdemo/assets/skybox.h>
#include <igdemo/render/camera.h>
#include <igdemo/render/ctx-components.h>
#include <igdemo/render/world-transform-component.h>
#include <igdemo/systems/pbr-geo-pass.h>

#include <glm/gtc/matrix_transform.hpp>

namespace igdemo {

const igecs::WorldView::Decl& PbrUploadSceneBuffersSystem::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           // Define outside of system
                                           .ctx_reads<CtxWgpuDevice>()
                                           .ctx_reads<CtxGeneralSceneParams>()
                                           .ctx_reads<CtxActiveCamera>()
                                           .ctx_reads<CtxHdrPassOutput>()

                                           // Defined inside of system
                                           .ctx_writes<CtxSceneLightingParams>()
                                           .ctx_writes<CtxGeneral3dBuffers>()

                                           // Individual entity read
                                           .reads<CameraComponent>();

  return decl;
}

void PbrUploadSceneBuffersSystem::run(igecs::WorldView* wv) {
  const auto& queue = wv->ctx<CtxWgpuDevice>().queue;
  const auto& generalSceneParams = wv->ctx<CtxGeneralSceneParams>();
  const auto& mainCameraEntity = wv->ctx<CtxActiveCamera>().activeCameraEntity;
  const auto& ctxHdrPassOutput = wv->ctx<CtxHdrPassOutput>();
  auto& sceneLightingParams = wv->mut_ctx<CtxSceneLightingParams>();
  auto& general3dBuffers = wv->mut_ctx<CtxGeneral3dBuffers>();

  const auto& mainCamera = wv->read<CameraComponent>(mainCameraEntity);

  float aspect_ratio = static_cast<float>(ctxHdrPassOutput.width) /
                       static_cast<float>(ctxHdrPassOutput.height);

  glm::mat4 mat_view =
      glm::lookAt(mainCamera.position, mainCamera.lookAt, mainCamera.up);
  glm::mat4 mat_proj = glm::perspective(mainCamera.fovy, aspect_ratio,
                                        mainCamera.nearPlaneDistance,
                                        mainCamera.farPlaneDistance);

  sceneLightingParams.cameraParams.cameraPos = mainCamera.position;
  sceneLightingParams.cameraParams.matViewProj = mat_proj * mat_view;
  sceneLightingParams.lightingParams.ambientCoefficient =
      generalSceneParams.ambientCoefficient;
  sceneLightingParams.lightingParams.sunColor = generalSceneParams.sunColor;
  sceneLightingParams.lightingParams.sunDirection =
      generalSceneParams.sunDirection;

  // TODO (sessamekesh): These only need to update if changed, which can be
  //  handled via an event firing? Optimize if needed.
  general3dBuffers.update_camera(queue, sceneLightingParams.cameraParams);
  general3dBuffers.update_lighting(queue, sceneLightingParams.lightingParams);
}

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
  wv->attach_ctx<CtxSceneLightingParams>();

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
          .ctx_reads<CtxHdrSkybox>()

          // DEFINED BY SYSTEM
          .ctx_reads<CtxAnimatedPbrPipeline>()
          .ctx_reads<AnimatedPbrFrameBindGroup>()

          // COMPONENT ITERATORS
          .reads<AnimatedPbrInstance>()
          .reads<AnimatedPbrSkinBindGroup>();

  return decl;
}

void PbrGeoPassSystem::run(igecs::WorldView* wv) {
  const auto& ctxWgpuDevice = wv->ctx<CtxWgpuDevice>();
  const auto& sceneLightingParams = wv->ctx<CtxSceneLightingParams>();
  auto& generalBuffers = wv->mut_ctx<CtxGeneral3dBuffers>();
  const auto& hdrOutView = wv->ctx<CtxHdrPassOutput>().hdrColorTextureView;
  const auto& dsView = wv->ctx<CtxHdrPassOutput>().dsView;
  const auto& ctxPipeline = wv->ctx<CtxAnimatedPbrPipeline>();
  const auto& ctxAnimatedPbrBindGroup = wv->ctx<AnimatedPbrFrameBindGroup>();
  const auto& ctxSkybox = wv->ctx<CtxHdrSkybox>();

  const auto& device = ctxWgpuDevice.device;
  const auto& queue = ctxWgpuDevice.queue;

  generalBuffers.update_camera(queue, sceneLightingParams.cameraParams);
  generalBuffers.update_lighting(queue, sceneLightingParams.lightingParams);

  wgpu::RenderPassColorAttachment ca{};
  ca.clearValue = {0.f, 0.f, 0.f, 1.f};
  ca.loadOp = wgpu::LoadOp::Load;
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
        pass.SetBindGroup(3, ctxSkybox.iblBindGroup.bindGroup);
        pass.DrawIndexed(geo->numIndices);
      }
    }

    pass.End();
  }
  wgpu::CommandBuffer commands = encoder.Finish();
  queue.Submit(1, &commands);
}

}  // namespace igdemo
