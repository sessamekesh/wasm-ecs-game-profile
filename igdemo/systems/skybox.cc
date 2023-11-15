#include <igdemo/render/bg-skybox.h>
#include <igdemo/render/camera.h>
#include <igdemo/render/ctx-components.h>
#include <igdemo/systems/skybox.h>

#include <glm/gtc/matrix_transform.hpp>

namespace igdemo {

const igecs::WorldView::Decl& SkyboxRenderSystem::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           // Define outside of system
                                           .ctx_reads<CtxWgpuDevice>()
                                           .ctx_writes<BgSkyboxPipeline>()
                                           .ctx_writes<CtxHdrPassOutput>()
                                           .ctx_reads<CtxActiveCamera>()
                                           .ctx_reads<CubemapUnitCube>()

                                           // Individual entity reads
                                           .reads<CameraComponent>();

  return decl;
}

void SkyboxRenderSystem::run(igecs::WorldView* wv) {
  const auto& ctxDevice = wv->ctx<CtxWgpuDevice>();
  auto& bgPipeline = wv->mut_ctx<BgSkyboxPipeline>();
  const auto& hdrOutParams = wv->ctx<CtxHdrPassOutput>();
  const auto& unitCubeGeo = wv->ctx<CubemapUnitCube>();

  const auto& mainCameraEntity = wv->ctx<CtxActiveCamera>().activeCameraEntity;
  const auto& mainCamera = wv->read<CameraComponent>(mainCameraEntity);

  float aspect_ratio = static_cast<float>(hdrOutParams.width) /
                       static_cast<float>(hdrOutParams.height);

  glm::mat4 mat_view =
      glm::lookAt(mainCamera.position, mainCamera.lookAt, mainCamera.up);
  glm::mat4 mat_proj = glm::perspective(mainCamera.fovy, aspect_ratio,
                                        mainCamera.nearPlaneDistance,
                                        mainCamera.farPlaneDistance);

  bgPipeline.set_camera_params(ctxDevice.queue, mat_view, mat_proj);

  wgpu::RenderPassColorAttachment ca{};
  ca.clearValue = {0.f, 0.f, 0.f, 1.f};
  ca.loadOp = wgpu::LoadOp::Load;
  ca.storeOp = wgpu::StoreOp::Store;
  ca.view = hdrOutParams.hdrColorTextureView;

  wgpu::RenderPassDepthStencilAttachment ds{};
  ds.depthClearValue = 1.f;
  ds.depthLoadOp = wgpu::LoadOp::Clear;
  ds.depthStoreOp = wgpu::StoreOp::Discard;
  ds.view = hdrOutParams.dsView;

  wgpu::RenderPassDescriptor rpd{};
  rpd.colorAttachmentCount = 1;
  rpd.colorAttachments = &ca;
  rpd.depthStencilAttachment = &ds;

  wgpu::CommandEncoder encoder = ctxDevice.device.CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&rpd);
  bgPipeline.add_to_render_pass(pass, unitCubeGeo);
  pass.End();
  auto commands = encoder.Finish();
  ctxDevice.queue.Submit(1, &commands);
}

}  // namespace igdemo
