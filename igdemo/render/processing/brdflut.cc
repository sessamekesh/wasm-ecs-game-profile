#include <igdemo/render/processing/brdflut.h>

namespace igdemo {

BRDFLutGenerator BRDFLutGenerator::Create(const wgpu::Device& device,
                                          const IgAsset::WgslSource* wgsl) {
  auto shader_module =
      create_shader_module(device, wgsl->source()->str(), "brdf-lut-shader");

  wgpu::VertexAttribute vPos{};
  vPos.format = wgpu::VertexFormat::Float32x3;
  vPos.offset = 0;
  vPos.shaderLocation = 0;

  wgpu::VertexAttribute vUv{};
  vUv.format = wgpu::VertexFormat::Float32x2;
  vUv.offset = sizeof(float) * 3;
  vUv.shaderLocation = 1;

  wgpu::VertexAttribute attribs[] = {vPos, vUv};
  wgpu::VertexBufferLayout vbl{};
  vbl.arrayStride = sizeof(float) * 5;
  vbl.attributeCount = 2;
  vbl.attributes = attribs;

  wgpu::ColorTargetState cts{};
  cts.format = wgpu::TextureFormat::RG16Float;

  wgpu::FragmentState fs{};
  fs.module = shader_module;
  fs.entryPoint = wgsl->fragment_entry_point()->c_str();
  fs.targetCount = 1;
  fs.targets = &cts;

  wgpu::RenderPipelineDescriptor rpd{};
  rpd.vertex.entryPoint = wgsl->vertex_entry_point()->c_str();
  rpd.vertex.module = shader_module;
  rpd.vertex.bufferCount = 1;
  rpd.vertex.buffers = &vbl;
  rpd.label = "brdf-lut-pipeline";
  rpd.fragment = &fs;
  rpd.primitive.topology = wgpu::PrimitiveTopology::TriangleStrip;
  rpd.primitive.cullMode = wgpu::CullMode::None;
  rpd.primitive.frontFace = wgpu::FrontFace::CCW;

  wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&rpd);

  return BRDFLutGenerator(pipeline);
}

TextureWithMeta BRDFLutGenerator::generate(const wgpu::Device& device,
                                           const wgpu::Queue& queue,
                                           const FullscreenQuad& quad_geo,
                                           std::uint32_t lut_width,
                                           const char* label) const {
  wgpu::TextureDescriptor td{};
  td.dimension = wgpu::TextureDimension::e2D;
  td.format = wgpu::TextureFormat::RG16Float;
  td.label = label;
  td.size = wgpu::Extent3D{lut_width, lut_width, 1};
  td.usage = wgpu::TextureUsage::RenderAttachment |
             wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
  auto texture = device.CreateTexture(&td);

  wgpu::RenderPassColorAttachment rpca{};
  rpca.clearValue = wgpu::Color{0.f, 0.f, 0.f, 1.f};
  rpca.loadOp = wgpu::LoadOp::Clear;
  rpca.storeOp = wgpu::StoreOp::Store;
  rpca.view = texture.CreateView();

  wgpu::RenderPassDescriptor rpd{};
  rpd.colorAttachmentCount = 1;
  rpd.colorAttachments = &rpca;

  auto enc = device.CreateCommandEncoder();
  auto pass = enc.BeginRenderPass(&rpd);
  pass.SetPipeline(pipeline_);
  pass.SetVertexBuffer(0, quad_geo.vertexBuffer.buffer, 0,
                       quad_geo.vertexBuffer.size);
  pass.Draw(4);
  pass.End();

  auto commands = enc.Finish();
  queue.Submit(1, &commands);

  return TextureWithMeta{texture, lut_width, lut_width,
                         wgpu::TextureFormat::RG16Float};
}

}  // namespace igdemo
