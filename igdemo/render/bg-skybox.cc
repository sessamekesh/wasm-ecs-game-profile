#include <igdemo/render/bg-skybox.h>
#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

BgSkyboxPipeline BgSkyboxPipeline::Create(
    const wgpu::Device& device, const wgpu::Queue& queue,
    const IgAsset::WgslSource* wgsl, const wgpu::TextureView& cubemapView) {
  auto shader_module =
      create_shader_module(device, wgsl->source()->str(), "bg-skybox-shader");

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
  fs.entryPoint = wgsl->fragment_entry_point()->c_str();
  fs.targetCount = 1;
  fs.targets = &color_target_state;

  wgpu::DepthStencilState dss{};
  dss.format = wgpu::TextureFormat::Depth32Float;
  dss.depthCompare = wgpu::CompareFunction::LessEqual;
  dss.depthWriteEnabled = false;

  wgpu::RenderPipelineDescriptor rpd{};
  rpd.vertex.bufferCount = 1;
  rpd.vertex.buffers = &vbl;
  rpd.vertex.entryPoint = wgsl->vertex_entry_point()->c_str();
  rpd.vertex.module = shader_module;
  rpd.label = "bg-skybox-pipeline";
  rpd.fragment = &fs;
  rpd.primitive.topology = wgpu::PrimitiveTopology::TriangleList;
  rpd.primitive.cullMode = wgpu::CullMode::None;
  rpd.primitive.frontFace = wgpu::FrontFace::CCW;
  rpd.depthStencil = &dss;

  wgpu::RenderPipeline pipeline = device.CreateRenderPipeline(&rpd);
  wgpu::BindGroupLayout cameraParamsBgl = pipeline.GetBindGroupLayout(0);
  wgpu::BindGroupLayout cubemapBgl = pipeline.GetBindGroupLayout(1);

  auto cameraParamsBuffer = create_buffer(device, queue, CameraParamsGPU{},
                                          wgpu::BufferUsage::Uniform,
                                          "bg-skybox-camera-params-buffer");

  wgpu::BindGroupEntry cameraBge{};
  cameraBge.binding = 0;
  cameraBge.buffer = cameraParamsBuffer;
  wgpu::BindGroupDescriptor cameraParamsBgd{};
  cameraParamsBgd.entries = &cameraBge;
  cameraParamsBgd.entryCount = 1;
  cameraParamsBgd.layout = cameraParamsBgl;

  wgpu::BindGroup cameraParamsBg = device.CreateBindGroup(&cameraParamsBgd);

  wgpu::BindGroupEntry samplerBge{};
  samplerBge.binding = 0;
  samplerBge.sampler = device.CreateSampler();

  wgpu::BindGroupEntry texBge{};
  texBge.binding = 1;
  texBge.textureView = cubemapView;

  wgpu::BindGroupEntry cubemapBindGroupEntries[] = {samplerBge, texBge};
  wgpu::BindGroupDescriptor cubemapBgd{};
  cubemapBgd.entries = cubemapBindGroupEntries;
  cubemapBgd.entryCount = 2;
  cubemapBgd.layout = cubemapBgl;

  wgpu::BindGroup cubemapBg = device.CreateBindGroup(&cubemapBgd);

  return BgSkyboxPipeline(pipeline, cameraParamsBgl, cubemapBgl,
                          cameraParamsBuffer, cameraParamsBg, cubemapBg);
}

void BgSkyboxPipeline::set_camera_params(const wgpu::Queue& queue,
                                         const glm::mat4& matView,
                                         const glm::mat4& matProj) {
  CameraParamsGPU gpuParams{matView, matProj};
  queue.WriteBuffer(cameraParamsBuffer, 0, &gpuParams, sizeof(CameraParamsGPU));
}

void BgSkyboxPipeline::add_to_render_pass(const wgpu::RenderPassEncoder& pass,
                                          const CubemapUnitCube& cube_geo) {
  pass.SetPipeline(pipeline);
  pass.SetBindGroup(0, cameraParamsBg);
  pass.SetBindGroup(1, cubemapBg);
  pass.SetVertexBuffer(0, cube_geo.vertexBuffer.buffer, 0,
                       cube_geo.vertexBuffer.size);
  pass.Draw(cube_geo.numVertices);
}

}  // namespace igdemo
