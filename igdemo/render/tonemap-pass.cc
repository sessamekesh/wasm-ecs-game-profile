#include <igdemo/render/ctx-components.h>
#include <igdemo/render/tonemap-pass.h>

namespace igdemo {
const igecs::WorldView::Decl& TonemapPass::decl() {
  static igecs::WorldView::Decl decl =
      igecs::WorldView::Decl().ctx_reads<CtxWgpuDevice>();

  return decl;
}

void TonemapPass::run(igecs::WorldView* wv) {
  const auto& ctx_wgpu_device = wv->ctx<CtxWgpuDevice>();

  const auto& device = ctx_wgpu_device.device;
  const auto& queue = ctx_wgpu_device.queue;
  const auto& target = ctx_wgpu_device.renderTarget;

  wgpu::RenderPassColorAttachment ca{};
  ca.clearValue = {0.15f, 0.15f, 0.65f, 1.f};
  ca.loadOp = wgpu::LoadOp::Clear;
  ca.storeOp = wgpu::StoreOp::Store;
  ca.view = target;

  wgpu::RenderPassDescriptor rpd{};
  rpd.colorAttachmentCount = 1;
  rpd.colorAttachments = &ca;

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
  {
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&rpd);
    pass.End();
  }
  wgpu::CommandBuffer commands = encoder.Finish();
  queue.Submit(1, &commands);
}

}  // namespace igdemo
