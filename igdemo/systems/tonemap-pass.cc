#include <igdemo/render/ctx-components.h>
#include <igdemo/render/wgpu-helpers.h>
#include <igdemo/systems/tonemap-pass.h>

namespace {

struct CtxTonemapPipeline {
  wgpu::RenderPipeline pipeline;
  wgpu::BindGroup hdrSamplingBindGroup;
  wgpu::Buffer quad_buffer;
};

}  // namespace

namespace igdemo {

bool TonemapPass::setup(igecs::WorldView* wv, const wgpu::Device& device,
                        const wgpu::Queue& queue,
                        const IgAsset::WgslSource* aces_tonemap_shader_src,
                        wgpu::TextureFormat output_format) {
  if (!aces_tonemap_shader_src->vertex_entry_point() ||
      !aces_tonemap_shader_src->fragment_entry_point()) {
    return false;
  }

  std::string wgsl_src = aces_tonemap_shader_src->source()->str();

  wgpu::ShaderModule shader_module =
      create_shader_module(device, wgsl_src, "aces-tonemapping-shader-module");

  wgpu::VertexAttribute pos_attrib{};
  pos_attrib.format = wgpu::VertexFormat::Float32x2;
  pos_attrib.offset = 0;
  pos_attrib.shaderLocation = 0;

  wgpu::VertexBufferLayout vertex_buffer_layout{};
  vertex_buffer_layout.arrayStride = sizeof(glm::vec2);
  vertex_buffer_layout.stepMode = wgpu::VertexStepMode::Vertex;
  vertex_buffer_layout.attributeCount = 1;
  vertex_buffer_layout.attributes = &pos_attrib;

  wgpu::RenderPipelineDescriptor rpd{};
  rpd.vertex.module = shader_module;
  rpd.vertex.entryPoint =
      aces_tonemap_shader_src->vertex_entry_point()->c_str();
  rpd.vertex.bufferCount = 1;
  rpd.vertex.buffers = &vertex_buffer_layout;

  wgpu::ColorTargetState color_target_state{};
  color_target_state.format = output_format;

  wgpu::FragmentState fs{};
  fs.module = shader_module;
  fs.entryPoint = aces_tonemap_shader_src->fragment_entry_point()->c_str();
  fs.targetCount = 1;
  fs.targets = &color_target_state;

  rpd.fragment = &fs;

  rpd.label = "aces-tonemap-pipeline";
  rpd.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  rpd.primitive.cullMode = wgpu::CullMode::None;
  rpd.primitive.frontFace = wgpu::FrontFace::CCW;

  wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&rpd);
  wgpu::BindGroupLayout hdr_sampling_bgl = pipeline.GetBindGroupLayout(0);

  const auto& hdr_in_texture_view =
      wv->ctx<CtxHdrPassOutput>().hdrColorTextureView;

  wgpu::Sampler sampler = device.CreateSampler();

  wgpu::BindGroupEntry sampler_entry{};
  sampler_entry.sampler = sampler;
  sampler_entry.binding = 0;

  wgpu::BindGroupEntry texture_entry{};
  texture_entry.textureView = hdr_in_texture_view;
  texture_entry.binding = 1;

  wgpu::BindGroupEntry entries[] = {sampler_entry, texture_entry};

  wgpu::BindGroupDescriptor bgd{};
  bgd.entryCount = 2;
  bgd.entries = entries;
  bgd.layout = hdr_sampling_bgl;
  bgd.label = "tonemap-pass-hdr-sampling-bind-group";

  auto bind_group = device.CreateBindGroup(&bgd);

  glm::vec2 quad_buffer_vertices[] = {
      glm::vec2(-1.f, -1.f), glm::vec2(-1.f, 1.f), glm::vec2(1.f, -1.f),
      glm::vec2(1.f, 1.f),   glm::vec2(1.f, -1.f), glm::vec2(-1.f, 1.f),
  };
  wgpu::BufferDescriptor vbd{};
  vbd.label = "fullscreen-quad-vertices";
  vbd.usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst;
  vbd.size = sizeof(glm::vec2) * 6;
  wgpu::Buffer vb = device.CreateBuffer(&vbd);
  queue.WriteBuffer(vb, 0, quad_buffer_vertices, sizeof(glm::vec2) * 6);

  wv->attach_ctx<CtxTonemapPipeline>(
      CtxTonemapPipeline{pipeline, bind_group, vb});

  return true;
}

const igecs::WorldView::Decl& TonemapPass::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           .ctx_reads<CtxWgpuDevice>()
                                           .ctx_reads<CtxTonemapPipeline>();

  return decl;
}

void TonemapPass::run(igecs::WorldView* wv) {
  const auto& ctx_wgpu_device = wv->ctx<CtxWgpuDevice>();
  const auto& ctxTonemapPipeline = wv->ctx<CtxTonemapPipeline>();

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

    pass.SetPipeline(ctxTonemapPipeline.pipeline);
    pass.SetBindGroup(0, ctxTonemapPipeline.hdrSamplingBindGroup);
    pass.SetVertexBuffer(0, ctxTonemapPipeline.quad_buffer, 0,
                         sizeof(glm::vec2) * 6);
    pass.Draw(6);

    pass.End();
  }
  wgpu::CommandBuffer commands = encoder.Finish();
  queue.Submit(1, &commands);
}

}  // namespace igdemo
