#include <igdemo/render/processing/prefilter-env-gen.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace igdemo {

PrefilteredEnvironmentMapGen PrefilteredEnvironmentMapGen::Create(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const IgAsset::WgslSource* wgsl) {
  auto shader_module = create_shader_module(device, wgsl->source()->str(),
                                            "prefiltered-env-map-gen-shader");

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
  rpd.label = "prefiltered-env-map-gen-pipeline";
  rpd.fragment = &fs;
  rpd.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  rpd.primitive.cullMode = wgpu::CullMode::None;
  rpd.primitive.frontFace = wgpu::FrontFace::CCW;

  wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&rpd);

  auto cameraBgl = pipeline.GetBindGroupLayout(0);
  auto samplerBgl = pipeline.GetBindGroupLayout(1);
  auto pbrParamsBgl = pipeline.GetBindGroupLayout(2);

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

  return PrefilteredEnvironmentMapGen(pipeline, cameraBgl, samplerBgl,
                                      pbrParamsBgl, cameraParamsBuffer, mvpBg);
}

TextureWithMeta PrefilteredEnvironmentMapGen::generate(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const CubemapUnitCube& cube_geo,
    const wgpu::TextureView& input_texture_view,
    std::uint32_t input_texture_width, std::uint32_t output_texture_width,
    const char* label) const {
  wgpu::TextureDescriptor cubemap_desc{};
  cubemap_desc.dimension = wgpu::TextureDimension::e2D;
  cubemap_desc.format = wgpu::TextureFormat::RGBA16Float;
  cubemap_desc.label = label;
  cubemap_desc.size =
      wgpu::Extent3D{output_texture_width, output_texture_width, 6};
  cubemap_desc.usage = wgpu::TextureUsage::RenderAttachment |
                       wgpu::TextureUsage::CopyDst |
                       wgpu::TextureUsage::TextureBinding;
  cubemap_desc.mipLevelCount = get_num_mips(output_texture_width);
  auto cubemap_texture = device.CreateTexture(&cubemap_desc);

  wgpu::SamplerDescriptor sd{};
  sd.addressModeU = wgpu::AddressMode::ClampToEdge;
  sd.addressModeV = wgpu::AddressMode::ClampToEdge;
  sd.addressModeW = wgpu::AddressMode::ClampToEdge;
  sd.magFilter = wgpu::FilterMode::Linear;
  sd.minFilter = wgpu::FilterMode::Linear;
  sd.mipmapFilter = wgpu::MipmapFilterMode::Linear;
  wgpu::Sampler sampler = device.CreateSampler(&sd);

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
  sbgd.layout = skyboxSamplerBgl_;
  wgpu::BindGroup samplerBindGroup = device.CreateBindGroup(&sbgd);

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();

  struct AllPbrParams {
    float roughness;
    float inputTextureResolution;
  };
  AllPbrParams allPbrParams{};
  allPbrParams.inputTextureResolution = input_texture_width;

  auto pbr_params_buffer =
      create_buffer(device, queue, allPbrParams, wgpu::BufferUsage::Uniform,
                    "prefilter-env-gen-pbr-params-buffer");
  wgpu::BindGroupEntry pbrParamsBge{};
  pbrParamsBge.buffer = pbr_params_buffer;
  pbrParamsBge.binding = 0;
  wgpu::BindGroupDescriptor pbrParamsBgd{};
  pbrParamsBgd.entries = &pbrParamsBge;
  pbrParamsBgd.entryCount = 1;
  pbrParamsBgd.label = "prefilter-env-gen-pbr-params-bind-group";
  pbrParamsBgd.layout = pbrParamsBgl_;
  wgpu::BindGroup pbrParamsBindGroup = device.CreateBindGroup(&pbrParamsBgd);

  std::uint32_t mip_levels = get_num_mips(output_texture_width);
  for (std::uint32_t mip = 0; mip < mip_levels; mip++) {
    wgpu::TextureViewDescriptor pxd{};
    pxd.arrayLayerCount = 1;
    pxd.baseArrayLayer = 0;
    pxd.dimension = wgpu::TextureViewDimension::e2D;
    pxd.format = wgpu::TextureFormat::RGBA16Float;
    pxd.baseMipLevel = mip;
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

    std::uint32_t mip_width = (128 * std::pow(0.5, mip));

    allPbrParams.roughness =
        static_cast<float>(mip) / static_cast<float>(mip_levels - 1);
    queue.WriteBuffer(pbr_params_buffer, 0, &allPbrParams,
                      sizeof(AllPbrParams));

    for (int i = 0; i < 6; i++) {
      wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&rpds[i]);
      pass.SetPipeline(pipeline_);
      pass.SetBindGroup(0, cameraParamsBindGroups_[i]);
      pass.SetBindGroup(1, samplerBindGroup);
      pass.SetBindGroup(2, pbrParamsBindGroup);
      pass.SetVertexBuffer(0, cube_geo.vertexBuffer.buffer, 0,
                           cube_geo.vertexBuffer.size);
      pass.Draw(cube_geo.numVertices);
      pass.End();
    }
  }

  auto commands = encoder.Finish();
  queue.Submit(1, &commands);

  return TextureWithMeta{cubemap_texture, output_texture_width,
                         output_texture_width,
                         wgpu::TextureFormat::RGBA16Float};
}

}  // namespace igdemo
