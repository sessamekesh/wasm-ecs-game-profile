#include <igdemo/render/animated-pbr.h>
#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

std::optional<CtxAnimatedPbrPipeline> CtxAnimatedPbrPipeline::Create(
    const igasset::IgpackDecoder& decoder, const std::string& wgsl_igasset_name,
    const wgpu::Device& device) {
  auto wgsl_result = decoder.extract_wgsl_shader(wgsl_igasset_name);
  if (std::holds_alternative<igasset::IgpackExtractError>(wgsl_result)) {
    // TODO (sessamekesh): log error
    return {};
  }

  const auto* wgsl_source = std::get<const IgAsset::WgslSource*>(wgsl_result);

  if (!wgsl_source->vertex_entry_point() ||
      !wgsl_source->fragment_entry_point()) {
    return {};
  }

  std::string wgsl_src = wgsl_source->source()->str();

  wgpu::ShaderModule shader_module =
      create_shader_module(device, wgsl_src, "animated-pbr-shader-module");

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

  wgpu::VertexAttribute bone_weight_attrib{};
  bone_weight_attrib.format = wgpu::VertexFormat::Float32x4;
  bone_weight_attrib.offset = offsetof(igasset::BoneWeightsVertexData, Weights);
  bone_weight_attrib.shaderLocation = 2;

  wgpu::VertexAttribute bone_index_attrib{};
  bone_index_attrib.format = wgpu::VertexFormat::Uint8x4;
  bone_index_attrib.offset = offsetof(igasset::BoneWeightsVertexData, Indices);
  bone_index_attrib.shaderLocation = 3;

  wgpu::VertexAttribute bone_weight_attributes[] = {bone_weight_attrib,
                                                    bone_index_attrib};

  wgpu::VertexBufferLayout bone_buffer_layout{};
  bone_buffer_layout.arrayStride = sizeof(igasset::BoneWeightsVertexData);
  bone_buffer_layout.stepMode = wgpu::VertexStepMode::Vertex;
  bone_buffer_layout.attributeCount = 2;
  bone_buffer_layout.attributes = bone_weight_attributes;

  wgpu::VertexBufferLayout vertex_buffer_layouts[] = {vertex_buffer_layout,
                                                      bone_buffer_layout};

  wgpu::RenderPipelineDescriptor rpd{};
  rpd.vertex.module = shader_module;
  rpd.vertex.entryPoint = wgsl_source->vertex_entry_point()->c_str();
  rpd.vertex.bufferCount = 2;
  rpd.vertex.buffers = vertex_buffer_layouts;

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

  return CtxAnimatedPbrPipeline{pipeline, frame_bgl, obj_bgl, inst_bgl};
}

AnimatedPbrFrameBindGroup::AnimatedPbrFrameBindGroup(
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

AnimatedPbrMaterial::AnimatedPbrMaterial(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const wgpu::BindGroupLayout& obj_bgl,
    const CtxAnimatedPbrPipeline::GPUPbrColorParams& material) {
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

AnimatedPbrSkinBindGroup::AnimatedPbrSkinBindGroup(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const wgpu::BindGroupLayout& skin_bgl, std::uint32_t num_bones) {
  skinMatrixBuffer = create_empty_vec_buffer<glm::mat4>(
      device, queue, wgpu::BufferUsage::Uniform, std::max(num_bones, 80u),
      nullptr);
  glm::mat4 i(1.f);
  worldTransformBuffer =
      create_buffer(device, queue, i, wgpu::BufferUsage::Uniform, nullptr);

  wgpu::BindGroupEntry skin_bge{};
  skin_bge.binding = 0;
  skin_bge.buffer = skinMatrixBuffer;

  wgpu::BindGroupEntry world_bge{};
  world_bge.binding = 1;
  world_bge.buffer = worldTransformBuffer;

  wgpu::BindGroupEntry entries[] = {skin_bge, world_bge};

  wgpu::BindGroupDescriptor bgd{};
  bgd.entries = entries;
  bgd.entryCount = 2;
  bgd.layout = skin_bgl;

  skinBindGroup = device.CreateBindGroup(&bgd);
}

void AnimatedPbrSkinBindGroup::update(const wgpu::Queue& queue,
                                      const std::vector<glm::mat4>& skin,
                                      const glm::mat4& worldTransform) const {
  queue.WriteBuffer(skinMatrixBuffer, 0, &skin[0],
                    skin.size() * sizeof(glm::mat4));
  queue.WriteBuffer(worldTransformBuffer, 0, &worldTransform,
                    sizeof(glm::mat4));
}

AnimatedPbrGeometry::AnimatedPbrGeometry(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const std::vector<igasset::PosNormalVertexData3D>& pos_norm_data,
    const std::vector<igasset::BoneWeightsVertexData>& bone_weights,
    const std::vector<std::uint16_t>& indices,
    std::vector<std::string> boneNames_, std::vector<glm::mat4> invBindPoses_)
    : vertexBuffer(nullptr),
      vertexBufferSize(0ul),
      boneWeightsBuffer(nullptr),
      boneWeightsBufferSize(0ul),
      indexBuffer(nullptr),
      indexBufferSize(0ul),
      numIndices(0ul),
      indexFormat(wgpu::IndexFormat::Undefined),
      boneNames(std::move(boneNames_)),
      invBindPoses(std::move(invBindPoses_)) {
  auto sized_vertex_buffer = create_vec_buffer(
      device, queue, pos_norm_data, wgpu::BufferUsage::Vertex, nullptr);
  auto sized_bone_buffer = create_vec_buffer(
      device, queue, bone_weights, wgpu::BufferUsage::Vertex, nullptr);
  auto sized_index_buffer = create_vec_buffer(
      device, queue, indices, wgpu::BufferUsage::Index, nullptr);

  vertexBuffer = sized_vertex_buffer.buffer;
  vertexBufferSize = sized_vertex_buffer.size;

  boneWeightsBuffer = sized_bone_buffer.buffer;
  boneWeightsBufferSize = sized_bone_buffer.size;

  indexBuffer = sized_index_buffer.buffer;
  indexBufferSize = sized_index_buffer.size;
  numIndices = indices.size();
  indexFormat = wgpu::IndexFormat::Uint16;
}

}  // namespace igdemo
