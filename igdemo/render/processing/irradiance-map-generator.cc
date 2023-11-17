#include <igdemo/render/processing/irradiance-map-generator.h>

#include <glm/gtc/matrix_transform.hpp>

namespace igdemo {

IrradianceMapGenerator IrradianceMapGenerator::Create(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const IgAsset::WgslSource* wgsl) {
  auto shader_module = create_shader_module(device, wgsl->source()->str(),
                                            "irradiance-map-shader");

  wgpu::VertexAttribute vPos{};
  vPos.format = wgpu::VertexFormat::Float32x3;
  vPos.offset = 0;
  vPos.shaderLocation = 0;

  wgpu::VertexBufferLayout vbl{};
  vbl.arrayStride = sizeof(float) * 3;
  vbl.attributeCount = 1;
  vbl.attributes = &vPos;

  wgpu::ColorTargetState cts{};
  cts.format = wgpu::TextureFormat::RGBA16Float;

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
  rpd.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  rpd.primitive.cullMode = wgpu::CullMode::None;
  rpd.primitive.frontFace = wgpu::FrontFace::CCW;

  wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&rpd);

  auto cameraBgl = pipeline.GetBindGroupLayout(0);
  auto samplerBgl = pipeline.GetBindGroupLayout(1);

  struct AllMvpViews {
    glm::mat4 pxv;
    glm::mat4 pxp;
    float __paddd_x_[32];
    glm::mat4 nxv;
    glm::mat4 nxp;
    float __paddd_nx_[32];
    glm::mat4 pyv;
    glm::mat4 pyp;
    float __paddd_y_[32];
    glm::mat4 nyv;
    glm::mat4 nyp;
    float __paddd_ny_[32];
    glm::mat4 pzv;
    glm::mat4 pzp;
    float __paddd_z_[32];
    glm::mat4 nzv;
    glm::mat4 nzp;
    float __paddd_nz_[32];
  };
  AllMvpViews views{};
  views.pxp = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  views.nxp = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  views.pyp = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  views.nyp = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  views.pzp = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
  views.nzp = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

  views.pxv =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f));
  views.nxv =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f));
  views.pyv =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, 1.0f));
  views.nyv =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                  glm::vec3(0.0f, 0.0f, -1.0f));
  views.pzv =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f));
  views.nzv =
      glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                  glm::vec3(0.0f, -1.0f, 0.0f));

  auto cameraParamsBuffer =
      create_buffer(device, queue, views, wgpu::BufferUsage::Uniform,
                    "irradiance-gen-camera-params-buffer");

  wgpu::BindGroupEntry bgepx{};
  bgepx.binding = 0;
  bgepx.buffer = cameraParamsBuffer;

  wgpu::BindGroupEntry bges[] = {bgepx, bgepx, bgepx, bgepx, bgepx, bgepx};
  wgpu::BindGroupDescriptor mvpBgd[6];
  wgpu::BindGroup mvpBg[6];
  for (int i = 0; i < 6; i++) {
    bges[i].offset = i * 256u;
    mvpBgd[i].entryCount = 1;
    mvpBgd[i].entries = &bges[i];
    mvpBgd[i].layout = cameraBgl;
    mvpBg[i] = device.CreateBindGroup(&mvpBgd[i]);
  }

  return IrradianceMapGenerator(pipeline, cameraBgl, samplerBgl,
                                cameraParamsBuffer, mvpBg);
}

IrradianceCubemap IrradianceMapGenerator::generate(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const CubemapUnitCube& cube_geo, const HdrMipsGenerator& mips_generator,
    const wgpu::TextureView& input_texture_view, std::uint32_t texture_width,
    const char* label) const {
  wgpu::TextureDescriptor cubemap_desc{};
  cubemap_desc.dimension = wgpu::TextureDimension::e2D;
  cubemap_desc.format = wgpu::TextureFormat::RGBA16Float;
  cubemap_desc.label = label;
  cubemap_desc.size = wgpu::Extent3D{texture_width, texture_width, 6};
  cubemap_desc.usage = wgpu::TextureUsage::RenderAttachment |
                       wgpu::TextureUsage::StorageBinding |
                       wgpu::TextureUsage::CopyDst |
                       wgpu::TextureUsage::TextureBinding;
  cubemap_desc.mipLevelCount = get_num_mips(texture_width);
  auto cubemap_texture = device.CreateTexture(&cubemap_desc);

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
  pyd.baseArrayLayer = 2;
  auto nyd = pxd;
  nyd.baseArrayLayer = 3;
  auto pzd = pxd;
  pzd.baseArrayLayer = 4;
  auto nzd = pxd;
  nzd.baseArrayLayer = 5;

  auto pxv = cubemap_texture.CreateView(&pxd);
  auto nxv = cubemap_texture.CreateView(&nxd);
  auto pyv = cubemap_texture.CreateView(&pyd);
  auto nyv = cubemap_texture.CreateView(&nyd);
  auto pzv = cubemap_texture.CreateView(&pzd);
  auto nzv = cubemap_texture.CreateView(&nzd);

  wgpu::TextureView views[] = {pxv, nxv, pyv, nyv, pzv, nzv};

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
  wgpu::RenderPassDescriptor rpds[] = {pxrpd, nxrpd, pyrpd,
                                       nyrpd, pzrpd, nzrpd};

  wgpu::Sampler sampler = device.CreateSampler();

  wgpu::BindGroupEntry samplerBge{};
  samplerBge.sampler = sampler;
  samplerBge.binding = 0;
  wgpu::BindGroupEntry hdrInputTextureBge{};
  hdrInputTextureBge.textureView = input_texture_view;
  hdrInputTextureBge.binding = 1;
  wgpu::BindGroupEntry sbgEntries[] = {samplerBge, hdrInputTextureBge};
  wgpu::BindGroupDescriptor sbgd{};
  sbgd.entries = sbgEntries;
  sbgd.entryCount = 2;
  sbgd.layout = samplerBgl_;
  wgpu::BindGroup sbg = device.CreateBindGroup(&sbgd);

  auto encoder = device.CreateCommandEncoder();
  for (auto i = 0; i < 6; i++) {
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&rpds[i]);
    pass.SetPipeline(pipeline_);
    pass.SetBindGroup(0, cameraParamsBindGroups_[i]);
    pass.SetBindGroup(1, sbg);
    pass.SetVertexBuffer(0, cube_geo.vertexBuffer.buffer, 0,
                         cube_geo.vertexBuffer.size);
    pass.Draw(cube_geo.numVertices);
    pass.End();
  }
  auto commands = encoder.Finish();
  queue.Submit(1, &commands);

  mips_generator.gen_cube_mips(device, queue, cubemap_texture, texture_width);

  return IrradianceCubemap{TextureWithMeta{cubemap_texture, texture_width,
                                           texture_width,
                                           wgpu::TextureFormat::RGBA16Float}};
}

}  // namespace igdemo
