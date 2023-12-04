#include <igdemo/render/static-pbr.h>
#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

std::optional<CtxStaticPbrPipeline> CtxStaticPbrPipeline::Create(
    const igasset::IgpackDecoder& decoder, const std::string& wgsl_igasset_name,
    const wgpu::Device& device) {
  auto wgsl_result = decoder.extract_wgsl_shader(wgsl_igasset_name);
  if (std::holds_alternative<igasset::IgpackExtractError>(wgsl_result)) {
    // TODO (sessamekesh): Log error
    return {};
  }

  const auto* wgsl_source = std::get<const IgAsset::WgslSource*>(wgsl_result);

  if (!wgsl_source->vertex_entry_point() ||
      !wgsl_source->fragment_entry_point()) {
    return {};
  }

  std::string wgsl_src = wgsl_source->source()->str();

  wgpu::ShaderModule shader_module =
      create_shader_module(device, wgsl_src, "static-pbr-shader-module");

  wgpu::VertexAttribute pos_attrib{};
  pos_attrib.format = wgpu::VertexFormat::Float32x3;
  pos_attrib.offset = offsetof(igasset::PosNormalVertexData3D, Position);
  pos_attrib.shaderLocation = 0;

  wgpu::VertexAttribute normal_attribute{};
  normal_attribute.format = wgpu::VertexFormat::Float32x3;
  normal_attribute.offset = offsetof(igasset::PosNormalVertexData3D, Normal);
  normal_attribute.shaderLocation = 1;

  wgpu::VertexAttribute vertex_buffer_attributes[] = {pos_attrib,
                                                      normal_attribute};

  wgpu::VertexBufferLayout vertex_buffer_layout{};
  vertex_buffer_layout.arrayStride = sizeof(igasset::PosNormalVertexData3D);
  vertex_buffer_layout.stepMode = wgpu::VertexStepMode::Vertex;
  vertex_buffer_layout.attributeCount = 2;
  vertex_buffer_layout.attributes = vertex_buffer_attributes;

  wgpu::RenderPipelineDescriptor rpd{};
  rpd.vertex.module = shader_module;
  rpd.vertex.entryPoint = wgsl_source->vertex_entry_point()->c_str();
  rpd.vertex.bufferCount = 1;
  rpd.vertex.buffers = &vertex_buffer_layout;

  wgpu::ColorTargetState color_target_state{};
  color_target_state.format = wgpu::TextureFormat::RGBA16Float;

  wgpu::FragmentState fs{};
  fs.module = shader_module;
  fs.entryPoint = wgsl_source->fragment_entry_point()->c_str();
  fs.targetCount = 1;
  fs.targets = &color_target_state;

  rpd.fragment = &fs;

  wgpu::DepthStencilState dss{};
  dss.format = wgpu::TextureFormat::Depth32Float;
  dss.depthCompare = wgpu::CompareFunction::Less;
  dss.depthWriteEnabled = true;

  rpd.depthStencil = &dss;
  rpd.label = "animated-pbr-pipeline";
  rpd.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  rpd.primitive.cullMode = wgpu::CullMode::Back;
  rpd.primitive.frontFace = wgpu::FrontFace::CCW;

  wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&rpd);
  wgpu::BindGroupLayout frame_bgl = pipeline.GetBindGroupLayout(0);
  wgpu::BindGroupLayout obj_bgl = pipeline.GetBindGroupLayout(1);
  wgpu::BindGroupLayout inst_bgl = pipeline.GetBindGroupLayout(2);
  wgpu::BindGroupLayout ibl_bgl = pipeline.GetBindGroupLayout(3);

  return CtxStaticPbrPipeline{pipeline, frame_bgl, obj_bgl, inst_bgl, ibl_bgl};
}

StaticPbrFrameBindGroup::StaticPbrFrameBindGroup(
    const wgpu::Device& device, const wgpu::BindGroupLayout& frame_bgl,
    const wgpu::Buffer& cameraParamsBuffer,
    const wgpu::Buffer& lightingParamsBuffer) {
  wgpu::BindGroupEntry camera_bge{};
  camera_bge.buffer = cameraParamsBuffer;
  camera_bge.binding = 0;

  wgpu::BindGroupEntry lighting_bge{};
  lighting_bge.buffer = lightingParamsBuffer;
  lighting_bge.binding = 1;

  wgpu::BindGroupEntry bge[] = {camera_bge, lighting_bge};

  wgpu::BindGroupDescriptor bgd{};
  bgd.entries = bge;
  bgd.entryCount = 2;
  bgd.layout = frame_bgl;

  frameBindGroup = device.CreateBindGroup(&bgd);
  cameraParams = cameraParamsBuffer;
  lightingParams = lightingParamsBuffer;
}

StaticPbrIblBindGroup::StaticPbrIblBindGroup(
    const wgpu::Device& device, const wgpu::BindGroupLayout& ibl_bgl,
    const wgpu::Texture& irradianceMap, const wgpu::Texture& prefilteredEnvMap,
    const wgpu::Texture& brdfLut) {
  wgpu::SamplerDescriptor samplerDesc{};
  samplerDesc.addressModeU = wgpu::AddressMode::ClampToEdge;
  samplerDesc.addressModeV = wgpu::AddressMode::ClampToEdge;
  samplerDesc.addressModeW = wgpu::AddressMode::ClampToEdge;
  samplerDesc.mipmapFilter = wgpu::MipmapFilterMode::Linear;
  samplerDesc.minFilter = wgpu::FilterMode::Linear;
  samplerDesc.magFilter = wgpu::FilterMode::Linear;

  wgpu::BindGroupEntry samplerBge{};
  samplerBge.binding = 0;
  samplerBge.sampler = device.CreateSampler(&samplerDesc);

  wgpu::TextureViewDescriptor irradianceMapViewDesc{};
  irradianceMapViewDesc.dimension = wgpu::TextureViewDimension::Cube;
  irradianceMapViewDesc.format = irradianceMap.GetFormat();
  irradianceMapViewDesc.mipLevelCount = irradianceMap.GetMipLevelCount();
  wgpu::TextureView irradianceView =
      irradianceMap.CreateView(&irradianceMapViewDesc);

  wgpu::BindGroupEntry irradianceBge{};
  irradianceBge.binding = 1;
  irradianceBge.textureView = irradianceView;

  wgpu::TextureViewDescriptor prefilterEnvMapViewDesc{};
  prefilterEnvMapViewDesc.dimension = wgpu::TextureViewDimension::Cube;
  prefilterEnvMapViewDesc.format = prefilteredEnvMap.GetFormat();
  prefilterEnvMapViewDesc.mipLevelCount = prefilteredEnvMap.GetMipLevelCount();
  wgpu::TextureView prefilteredEnvMapView =
      prefilteredEnvMap.CreateView(&prefilterEnvMapViewDesc);

  wgpu::BindGroupEntry prefilterBge{};
  prefilterBge.binding = 2;
  prefilterBge.textureView = prefilteredEnvMapView;

  wgpu::BindGroupEntry brdfLutBge{};
  brdfLutBge.binding = 3;
  brdfLutBge.textureView = brdfLut.CreateView();

  wgpu::BindGroupEntry entries[] = {samplerBge, irradianceBge, prefilterBge,
                                    brdfLutBge};

  wgpu::BindGroupDescriptor bgd{};
  bgd.entries = entries;
  bgd.entryCount = 4;
  bgd.layout = ibl_bgl;

  wgpu::BindGroup bg = device.CreateBindGroup(&bgd);

  bindGroup = bg;
  iblSampler = samplerBge.sampler;
  irradianceMapView = irradianceView;
  prefilteredEnvView = prefilteredEnvMapView;
  brdfLutView = brdfLutBge.textureView;
}

StaticPbrMaterial::StaticPbrMaterial(const wgpu::Device& device,
                                     const wgpu::Queue& queue,
                                     const wgpu::BindGroupLayout& obj_bgl,
                                     const pbr::GPUPbrColorParams& material) {
  materialBuffer = create_buffer(device, queue, material,
                                 wgpu::BufferUsage::Uniform, nullptr);

  wgpu::BindGroupEntry bge{};
  bge.binding = 0;
  bge.buffer = materialBuffer;

  wgpu::BindGroupDescriptor bgd{};
  bgd.entries = &bge;
  bgd.entryCount = 1;
  bgd.layout = obj_bgl;

  objBindGroup = device.CreateBindGroup(&bgd);
}

StaticPbrGeometry::StaticPbrGeometry(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const std::vector<igasset::PosNormalVertexData3D>& pos_norm_data,
    const std::vector<std::uint16_t>& indices)
    : vertexBuffer(nullptr),
      vertexBufferSize(0ul),
      indexBuffer(nullptr),
      indexBufferSize(0ul),
      numIndices(0ul),
      indexFormat(wgpu::IndexFormat::Undefined) {
  auto sized_vertex_buffer = create_vec_buffer(
      device, queue, pos_norm_data, wgpu::BufferUsage::Vertex, nullptr);
  auto sized_index_buffer = create_vec_buffer(
      device, queue, indices, wgpu::BufferUsage::Index, nullptr);

  vertexBuffer = sized_vertex_buffer.buffer;
  vertexBufferSize = sized_vertex_buffer.size;

  indexBuffer = sized_index_buffer.buffer;
  indexBufferSize = sized_index_buffer.size;
  numIndices = indices.size();
  indexFormat = wgpu::IndexFormat::Uint16;
}

StaticPbrModelBindGroup::StaticPbrModelBindGroup(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const wgpu::BindGroupLayout& model_bgl) {
  glm::mat4 i(1.f);
  worldTransformBuffer =
      create_buffer(device, queue, i, wgpu::BufferUsage::Uniform, nullptr);

  wgpu::BindGroupEntry world_bge{};
  world_bge.binding = 0;
  world_bge.buffer = worldTransformBuffer;

  wgpu::BindGroupDescriptor bgd{};
  bgd.entries = &world_bge;
  bgd.entryCount = 1;
  bgd.layout = model_bgl;

  bindGroup = device.CreateBindGroup(&bgd);
}

void StaticPbrModelBindGroup::update(const wgpu::Queue& queue,
                                     const glm::mat4& worldTransform) const {
  queue.WriteBuffer(worldTransformBuffer, 0, &worldTransform,
                    sizeof(glm::mat4));
}

}  // namespace igdemo
