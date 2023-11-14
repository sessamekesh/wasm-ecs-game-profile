#include <igdemo/render/processing/equirect-to-cubemap.h>
#include <igdemo/render/wgpu-helpers.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace igdemo {

EquirectangularToCubemapPipeline EquirectangularToCubemapPipeline::Create(
    const wgpu::Device& device, const IgAsset::WgslSource* wgsl_source) {
  auto shader_module =
      create_shader_module(device, wgsl_source->source()->str(),
                           "equirectangular-to-cubemap-shader");

  wgpu::VertexAttribute pos{};
  pos.format = wgpu::VertexFormat::Float32x3;
  pos.offset = 0;
  pos.shaderLocation = 0;
  wgpu::VertexBufferLayout vbl{};
  vbl.arrayStride = sizeof(glm::vec3);
  vbl.attributeCount = 1;
  vbl.attributes = &pos;

  wgpu::ColorTargetState color_target_state{};
  color_target_state.format = wgpu::TextureFormat::RGBA16Float;

  wgpu::FragmentState fs{};
  fs.module = shader_module;
  fs.entryPoint = wgsl_source->fragment_entry_point()->c_str();
  fs.targetCount = 1;
  fs.targets = &color_target_state;

  wgpu::RenderPipelineDescriptor rpd{};
  rpd.vertex.bufferCount = 0;
  rpd.vertex.buffers = nullptr;
  rpd.vertex.entryPoint = wgsl_source->vertex_entry_point()->c_str();
  rpd.vertex.module = shader_module;
  rpd.vertex.bufferCount = 1;
  rpd.vertex.buffers = &vbl;
  rpd.label = "equirectangular-to-cubemap-pipeline";
  rpd.fragment = &fs;
  rpd.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  rpd.primitive.cullMode = wgpu::CullMode::None;
  rpd.primitive.frontFace = wgpu::FrontFace::CCW;

  wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&rpd);
  wgpu::BindGroupLayout mvpBgl = pipeline.GetBindGroupLayout(0);
  wgpu::BindGroupLayout samplerBgl = pipeline.GetBindGroupLayout(1);

  return EquirectangularToCubemapPipeline(pipeline, mvpBgl, samplerBgl);
}

ConversionOutput EquirectangularToCubemapPipeline::convert(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const wgpu::TextureView& equirect, const CubemapUnitCube& unit_cube,
    std::uint32_t cubemap_face_width, wgpu::TextureUsage usage,
    const char* label) const {
  wgpu::TextureDescriptor cubemap_desc{};
  cubemap_desc.dimension = wgpu::TextureDimension::e2D;
  cubemap_desc.format = wgpu::TextureFormat::RGBA16Float;
  cubemap_desc.label = label;
  cubemap_desc.size = wgpu::Extent3D{cubemap_face_width, cubemap_face_width, 6};
  cubemap_desc.usage = wgpu::TextureUsage::RenderAttachment |
                       wgpu::TextureUsage::CopyDst | usage;
  auto cubemap_texture = device.CreateTexture(&cubemap_desc);

  wgpu::TextureViewDescriptor pxd{};
  pxd.arrayLayerCount = 1;
  pxd.baseArrayLayer = 0;
  pxd.dimension = wgpu::TextureViewDimension::e2D;
  pxd.format = wgpu::TextureFormat::RGBA16Float;
  pxd.baseMipLevel = 0;
  pxd.mipLevelCount = 1;

  auto nxd = pxd;
  nxd.baseArrayLayer = 1;
  auto pyd = pxd;
  nxd.baseArrayLayer = 2;
  auto nyd = pxd;
  nxd.baseArrayLayer = 3;
  auto pzd = pxd;
  nxd.baseArrayLayer = 4;
  auto nzd = pxd;
  nxd.baseArrayLayer = 5;

  auto pxv = cubemap_texture.CreateView(&pxd);
  auto nxv = cubemap_texture.CreateView(&nxd);
  auto pyv = cubemap_texture.CreateView(&pyd);
  auto nyv = cubemap_texture.CreateView(&nyd);
  auto pzv = cubemap_texture.CreateView(&pzd);
  auto nzv = cubemap_texture.CreateView(&nzd);

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

  wgpu::RenderPassColorAttachment pxca{};
  pxca.clearValue = wgpu::Color{0.f, 0.f, 0.f, 0.f};
  pxca.loadOp = wgpu::LoadOp::Clear;
  pxca.storeOp = wgpu::StoreOp::Store;
  pxca.view = pxv;

  wgpu::RenderPassColorAttachment nxca = pxca;
  nxca.view = nxv;
  wgpu::RenderPassColorAttachment pyca = pxca;
  pyca.view = pyv;
  wgpu::RenderPassColorAttachment nyca = pxca;
  nyca.view = nyv;
  wgpu::RenderPassColorAttachment pzca = pxca;
  pzca.view = pzv;
  wgpu::RenderPassColorAttachment nzca = pxca;
  nzca.view = nzv;

  wgpu::RenderPassDescriptor pxrpd{};
  pxrpd.colorAttachmentCount = 1;
  pxrpd.colorAttachments = &pxca;
  wgpu::RenderPassDescriptor nxrpd{};
  nxrpd.colorAttachmentCount = 1;
  nxrpd.colorAttachments = &nxca;
  wgpu::RenderPassDescriptor pyrpd{};
  pyrpd.colorAttachmentCount = 1;
  pyrpd.colorAttachments = &pyca;
  wgpu::RenderPassDescriptor nyrpd{};
  nyrpd.colorAttachmentCount = 1;
  nyrpd.colorAttachments = &nyca;
  wgpu::RenderPassDescriptor pzrpd{};
  pzrpd.colorAttachmentCount = 1;
  pzrpd.colorAttachments = &pzca;
  wgpu::RenderPassDescriptor nzrpd{};
  nzrpd.colorAttachmentCount = 1;
  nzrpd.colorAttachments = &nzca;

  glm::mat4 captureProjection =
      glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  glm::mat4 captureViews[] = {
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, -1.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f)),
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f))};

  glm::mat4 mvp = captureProjection * captureViews[0];

  wgpu::Buffer mvpBuffer =
      create_buffer(device, queue, mvp, wgpu::BufferUsage::Uniform, nullptr);

  wgpu::Sampler sampler = device.CreateSampler();

  wgpu::BindGroupEntry mvpBge{};
  mvpBge.binding = 0;
  mvpBge.buffer = mvpBuffer;
  wgpu::BindGroupDescriptor mvpBgd{};
  mvpBgd.entryCount = 1;
  mvpBgd.entries = &mvpBge;
  wgpu::BindGroup mvpBg = device.CreateBindGroup(&mvpBgd);

  wgpu::BindGroupEntry samplerBge{};
  samplerBge.sampler = sampler;
  samplerBge.binding = 0;
  wgpu::BindGroupEntry equirectTextureBge{};
  equirectTextureBge.textureView = equirect;
  equirectTextureBge.binding = 1;
  wgpu::BindGroupEntry sbgEntries[] = {samplerBge, equirectTextureBge};
  wgpu::BindGroupDescriptor sbgd{};
  sbgd.entries = sbgEntries;
  sbgd.entryCount = 2;
  wgpu::BindGroup sbg = device.CreateBindGroup(&sbgd);

  const auto& ucvb = unit_cube.vertexBuffer;
  int ucnv = unit_cube.numVertices;

  wgpu::RenderPassEncoder pxp = encoder.BeginRenderPass(&pxrpd);
  pxp.SetPipeline(pipeline_);
  pxp.SetBindGroup(0, mvpBg);
  pxp.SetBindGroup(1, sbg);
  pxp.SetVertexBuffer(0, ucvb.buffer, 0, ucvb.size);
  pxp.Draw(ucnv, 1, 0, 0);
  pxp.End();

  mvp = captureProjection * captureViews[1];
  queue.WriteBuffer(mvpBuffer, 0, &mvp, sizeof(glm::mat4));
  wgpu::RenderPassEncoder nxp = encoder.BeginRenderPass(&nxrpd);
  nxp.SetPipeline(pipeline_);
  nxp.SetBindGroup(0, mvpBg);
  nxp.SetBindGroup(1, sbg);
  pxp.SetVertexBuffer(0, ucvb.buffer, 0, ucvb.size);
  pxp.Draw(ucnv, 1, 0, 0);
  nxp.End();

  mvp = captureProjection * captureViews[2];
  queue.WriteBuffer(mvpBuffer, 0, &mvp, sizeof(glm::mat4));
  wgpu::RenderPassEncoder pyp = encoder.BeginRenderPass(&pyrpd);
  pyp.SetPipeline(pipeline_);
  pyp.SetBindGroup(0, mvpBg);
  pyp.SetBindGroup(1, sbg);
  pxp.Draw(ucnv, 1, 0, 0);
  pyp.End();

  mvp = captureProjection * captureViews[3];
  queue.WriteBuffer(mvpBuffer, 0, &mvp, sizeof(glm::mat4));
  wgpu::RenderPassEncoder nyp = encoder.BeginRenderPass(&nyrpd);
  nyp.SetPipeline(pipeline_);
  nyp.SetBindGroup(0, mvpBg);
  nyp.SetBindGroup(1, sbg);
  pxp.SetVertexBuffer(0, ucvb.buffer, 0, ucvb.size);
  pxp.Draw(ucnv, 1, 0, 0);
  nyp.End();

  mvp = captureProjection * captureViews[4];
  queue.WriteBuffer(mvpBuffer, 0, &mvp, sizeof(glm::mat4));
  wgpu::RenderPassEncoder pzp = encoder.BeginRenderPass(&pzrpd);
  pzp.SetPipeline(pipeline_);
  pzp.SetBindGroup(0, mvpBg);
  pzp.SetBindGroup(1, sbg);
  pxp.SetVertexBuffer(0, ucvb.buffer, 0, ucvb.size);
  pxp.Draw(ucnv, 1, 0, 0);
  pzp.End();

  mvp = captureProjection * captureViews[5];
  queue.WriteBuffer(mvpBuffer, 0, &mvp, sizeof(glm::mat4));
  wgpu::RenderPassEncoder nzp = encoder.BeginRenderPass(&nzrpd);
  nzp.SetPipeline(pipeline_);
  nzp.SetBindGroup(0, mvpBg);
  nzp.SetBindGroup(1, sbg);
  pxp.SetVertexBuffer(0, ucvb.buffer, 0, ucvb.size);
  pxp.Draw(ucnv, 1, 0, 0);
  nzp.End();

  wgpu::CommandBuffer commands = encoder.Finish();

  queue.Submit(1, &commands);

  return ConversionOutput{cubemap_texture, cubemap_texture.CreateView()};
}

}  // namespace igdemo
